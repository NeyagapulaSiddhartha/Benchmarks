#!/bin/bash

# ============================================================
# Benchmark Analysis Script — ASan + Output Anomaly Detection
# Usage: ./run_benchmarks.sh <clang++_path> <benchmark_dir>
# Example: ./run_benchmarks.sh \
#   ~/FINDUS_Artifact/FINDUS/llvm-16.0.0.obj/bin/clang++ \
#   ~/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/PTHREAD_VERSION
# ============================================================

CLANGPP="${1:-clang++}"
BENCH_DIR="${2:-.}"
RUNS="${3:-5}"          # how many times to run each benchmark
ASAN_FLAGS="-fsanitize=address -g -fno-omit-frame-pointer"
LINK_FLAGS="-pthread"

# Thresholds
LARGE_NUM_THRESHOLD=1000          # absolute value above this = anomalous
NEGATIVE_NUM_THRESHOLD= -100      # or below this

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

BINARY="/tmp/asan_bench_binary"
REPORT_FILE="benchmark_report_$(date +%Y%m%d_%H%M%S).txt"
SKIPPED_FILE="skipped_user_input_$(date +%Y%m%d_%H%M%S).txt"

# Patterns that indicate the program reads user input at runtime.
# grep -E, so these are extended-regex patterns.
USER_INPUT_PATTERNS=(
    '\bcin\s*>>'                  # cin >> var
    '\bgetline\s*\('              # getline(cin, ...)
    '\bscanf\s*\('                # scanf(...)
    '\bfscanf\s*\(stdin'          # fscanf(stdin, ...)
    '\bfgets\s*\('                # fgets(buf, n, stdin)
    '\bgetchar\s*\('              # getchar()
    '\bgets\s*\('                 # gets() — unsafe but exists
    '\bread\s*\(\s*STDIN_FILENO'  # read(STDIN_FILENO, ...)
    '\bgetc\s*\(\s*stdin'         # getc(stdin)
    '\bfgetc\s*\(\s*stdin'        # fgetc(stdin)
    'std::cin'                    # explicit namespace form
    'cin'                    # explicit namespace form
    '\bargc\b.*\bargv\b'          # uses command-line arguments
    '\bargv\s*\['                 # argv[i] access
)

# ============================================================
# Helpers
# ============================================================

log()   { echo -e "$*" | tee -a "$REPORT_FILE"; }
hr()    { log "${CYAN}$(printf '─%.0s' {1..70})${NC}"; }

# Check if output contains suspiciously large numbers
contains_large_numbers() {
    local output="$1"
    # Extract all integers from the output
    while IFS= read -r num; do
        [[ -z "$num" ]] && continue
        if (( num > LARGE_NUM_THRESHOLD || num < NEGATIVE_NUM_THRESHOLD )); then
            return 0   # found anomalous number
        fi
    done < <(echo "$output" | grep -oE '\-?[0-9]{6,}')
    return 1
}

# Classify ASan stderr output
classify_asan() {
    local asan_err="$1"
    if echo "$asan_err" | grep -qiE 'ERROR: AddressSanitizer'; then
        echo "ASAN_CAUGHT"
    elif echo "$asan_err" | grep -qiE 'WARNING: AddressSanitizer'; then
        echo "ASAN_WARNING"
    elif echo "$asan_err" | grep -qiE 'Segmentation fault|SIGSEGV|signal 11'; then
        echo "SEGFAULT"
    else
        echo "ASAN_CLEAN"
    fi
}

# ============================================================
# Scan a .cpp file for user-input constructs.
# Returns 0 (true) if input detected, 1 (false) if clean.
# Echoes "pattern|||linematch" when found.
# ============================================================
takes_user_input() {
    local src="$1"
    for pattern in "${USER_INPUT_PATTERNS[@]}"; do
        local match
        match=$(grep -nE "$pattern" "$src" 2>/dev/null | head -1)
        if [[ -n "$match" ]]; then
            echo "${pattern}|||${match}"
            return 0
        fi
    done
    return 1
}

# ============================================================
# Compile one file, return 0 on success
# ============================================================
compile_file() {
    local src="$1"
    local compile_err
    compile_err=$("$CLANGPP" $ASAN_FLAGS $LINK_FLAGS "$src" -o "$BINARY" 2>&1)
    local rc=$?
    echo "$compile_err"
    return $rc
}

# ============================================================
# Analyse one benchmark file
# ============================================================
analyse_benchmark() {
    local src="$1"
    local filename
    filename=$(basename "$src")

    hr
    log "${BOLD}▶  FILE: $filename${NC}"
    log "   Path: $src"
    log ""

    # --- Pre-flight: scan for user-input constructs ---
    local scan_result
    scan_result=$(takes_user_input "$src")
    if [[ $? -eq 0 ]]; then
        local matched_pattern matched_line
        matched_pattern="${scan_result%%|||*}"
        matched_line="${scan_result##*|||}"
        log "  ${YELLOW}[SKIPPED]${NC} File reads user input — cannot run unattended."
        log "  ${YELLOW}  Pattern matched : ${matched_pattern}${NC}"
        log "  ${YELLOW}  At line         : ${matched_line}${NC}"
        log ""
        # Record in skipped file
        {
            echo "FILE: $src"
            echo "  Pattern : $matched_pattern"
            echo "  Line    : $matched_line"
            echo ""
        } >> "$SKIPPED_FILE"
        RESULTS["$filename"]="SKIPPED_USER_INPUT"
        ((skipped_total++))
        return
    fi

    # --- Compile ---
    local compile_output
    compile_output=$(compile_file "$src")
    if [[ $? -ne 0 ]]; then
        log "${RED}  [COMPILE ERROR]${NC}"
        log "$compile_output"
        RESULTS["$filename"]="COMPILE_ERROR"
        return
    fi
    log "  ${GREEN}Compiled OK${NC}"

    # --- Run multiple times to expose non-deterministic bugs ---
    local asan_caught=0
    local segfault_count=0
    local large_num_count=0
    local clean_count=0
    local asan_output_sample=""

    for ((i=1; i<=RUNS; i++)); do
        # Capture stdout and stderr separately
        local stdout_file stderr_file
        stdout_file=$(mktemp)
        stderr_file=$(mktemp)

        timeout 15 "$BINARY" >"$stdout_file" 2>"$stderr_file"
        local exit_code=$?

        local stdout_content stderr_content
        stdout_content=$(cat "$stdout_file")
        stderr_content=$(cat "$stderr_file")

        rm -f "$stdout_file" "$stderr_file"

        local asan_class
        asan_class=$(classify_asan "$stderr_content")

        if [[ "$asan_class" == "ASAN_CAUGHT" || "$asan_class" == "ASAN_WARNING" ]]; then
            ((asan_caught++))
            [[ -z "$asan_output_sample" ]] && asan_output_sample="$stderr_content"
        elif [[ "$asan_class" == "SEGFAULT" ]]; then
            ((segfault_count++))
        elif contains_large_numbers "$stdout_content"; then
            ((large_num_count++))
            # Capture sample of anomalous output
            if [[ -z "$asan_output_sample" ]]; then
                asan_output_sample="$stdout_content"
            fi
        else
            ((clean_count++))
        fi
    done

    # --- Verdict ---
    log ""
    log "  ${BOLD}Results over $RUNS run(s):${NC}"
    log "    ASan error caught : $asan_caught"
    log "    Segfaults          : $segfault_count"
    log "    Large-number output: $large_num_count  ${YELLOW}(ASan missed, UAS visible in output)${NC}"
    log "    Clean runs         : $clean_count"
    log ""

    if (( asan_caught > 0 )); then
        log "  ${RED}${BOLD}[VERDICT] ❌ ASan DETECTED a bug${NC}"
        log ""
        # Print condensed ASan report (first ERROR line + stack summary)
        local asan_summary
        asan_summary=$(echo "$asan_output_sample" | grep -A 20 'ERROR: AddressSanitizer' | head -25)
        log "${RED}  --- ASan Report (excerpt) ---${NC}"
        log "$asan_summary"
        RESULTS["$filename"]="ASAN_CAUGHT"

    elif (( segfault_count > 0 && large_num_count == 0 && asan_caught == 0 )); then
        log "  ${RED}${BOLD}[VERDICT] 💥 SEGFAULT only — bug present, ASan did not report${NC}"
        RESULTS["$filename"]="SEGFAULT_ONLY"

    elif (( large_num_count > 0 )); then
        log "  ${YELLOW}${BOLD}[VERDICT] ⚠️  ASan MISSED the bug — garbage values observed in output${NC}"
        log ""
        log "${YELLOW}  --- Sample anomalous output ---${NC}"
        log "$(echo "$asan_output_sample" | head -15)"
        RESULTS["$filename"]="ASAN_MISSED"

    elif (( segfault_count > 0 )); then
        log "  ${RED}${BOLD}[VERDICT] 💥 Intermittent SEGFAULT + possible ASan miss${NC}"
        RESULTS["$filename"]="INTERMITTENT"

    else
        log "  ${GREEN}${BOLD}[VERDICT] ✅ Program appears to run correctly (no errors detected)${NC}"
        RESULTS["$filename"]="CLEAN"
    fi

    log ""
}

# ============================================================
# Main
# ============================================================

declare -A RESULTS
skipped_total=0

# Initialise skipped file with header
{
    echo "========================================"
    echo " Files Skipped — Require User Input"
    echo " Generated: $(date)"
    echo "========================================"
    echo ""
} > "$SKIPPED_FILE"

log ""
log "${BOLD}${CYAN}╔══════════════════════════════════════════════════════════════════╗${NC}"
log "${BOLD}${CYAN}║        MTUASBench — ASan Benchmark Analysis Script               ║${NC}"
log "${BOLD}${CYAN}╚══════════════════════════════════════════════════════════════════╝${NC}"
log ""
log "  Clang++    : $CLANGPP"
log "  Bench dir  : $BENCH_DIR"
log "  Runs/bench : $RUNS"
log "  ASan flags : $ASAN_FLAGS"
log "  Report     : $REPORT_FILE"
log "  Skipped    : $SKIPPED_FILE  (files requiring user input)"
log ""

# Verify clang++ exists
if ! command -v "$CLANGPP" &>/dev/null && [[ ! -x "$CLANGPP" ]]; then
    log "${RED}ERROR: clang++ not found at '$CLANGPP'${NC}"
    exit 1
fi

# Find all .cpp files recursively
mapfile -t CPP_FILES < <(find "$BENCH_DIR" -name "*.cpp" | sort)

if [[ ${#CPP_FILES[@]} -eq 0 ]]; then
    log "${RED}No .cpp files found in $BENCH_DIR${NC}"
    exit 1
fi

log "  Found ${#CPP_FILES[@]} benchmark file(s)"
log ""

for src in "${CPP_FILES[@]}"; do
    analyse_benchmark "$src"
done

# ============================================================
# Summary Table
# ============================================================
hr
log ""
log "${BOLD}${CYAN}SUMMARY${NC}"
log ""
printf "  %-60s %s\n" "Benchmark" "Verdict" | tee -a "$REPORT_FILE"
printf "  %-60s %s\n" "$(printf '─%.0s' {1..60})" "$(printf '─%.0s' {1..15})" | tee -a "$REPORT_FILE"

asan_caught_total=0
asan_missed_total=0
clean_total=0
compile_err_total=0
segfault_total=0
intermittent_total=0

for filename in "${!RESULTS[@]}"; do
    verdict="${RESULTS[$filename]}"
    case "$verdict" in
        ASAN_CAUGHT)        color=$RED;    icon="❌ ASan caught";       ((asan_caught_total++))  ;;
        ASAN_MISSED)        color=$YELLOW; icon="⚠️  ASan missed";       ((asan_missed_total++))  ;;
        CLEAN)              color=$GREEN;  icon="✅ Clean";              ((clean_total++))        ;;
        COMPILE_ERROR)      color=$RED;    icon="🔨 Compile error";     ((compile_err_total++))  ;;
        SEGFAULT_ONLY)      color=$RED;    icon="💥 Segfault";          ((segfault_total++))     ;;
        INTERMITTENT)       color=$YELLOW; icon="⚠️  Intermittent";      ((intermittent_total++)) ;;
        SKIPPED_USER_INPUT) color=$BLUE;   icon="⏭️  Skipped (input)";   ;;
    esac
    printf "  ${color}%-60s %s${NC}\n" "$filename" "$icon" | tee -a "$REPORT_FILE"
done

log ""
log "  ${BOLD}Totals:${NC}"
log "    ❌ ASan caught bugs  : $asan_caught_total"
log "    ⚠️  ASan missed bugs  : $((asan_missed_total + intermittent_total))"
log "    💥 Segfault only     : $segfault_total"
log "    ✅ Clean             : $clean_total"
log "    🔨 Compile errors    : $compile_err_total"
log "    ⏭️  Skipped (input)   : $skipped_total"
log ""
log "  Full report  : ${BOLD}$REPORT_FILE${NC}"
if (( skipped_total > 0 )); then
    log "  Skipped list : ${BOLD}$SKIPPED_FILE${NC}  ← review these manually"
fi
log ""