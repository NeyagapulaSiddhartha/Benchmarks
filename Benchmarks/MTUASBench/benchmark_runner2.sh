#!/bin/bash

# ============================================================
# Benchmark Analysis Script — ASan + Output Anomaly Detection
#                             PARALLEL VERSION
# Usage: ./run_benchmarks.sh <clang++_path> <benchmark_dir> [runs] [jobs]
# Example: ./run_benchmarks.sh \
#   ~/FINDUS_Artifact/FINDUS/llvm-16.0.0.obj/bin/clang++ \
#   ~/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/PTHREAD_VERSION \
#   10 8
# ============================================================

CLANGPP="${1:-clang++}"
BENCH_DIR="${2:-.}"
RUNS="${3:-5}"
MAX_JOBS="${4:-$(nproc)}"   # parallel workers; defaults to CPU core count

ASAN_FLAGS="-fsanitize=address -g -fno-omit-frame-pointer"
LINK_FLAGS="-pthread"

LARGE_NUM_THRESHOLD=100000
NEGATIVE_NUM_THRESHOLD=-100000

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

# All output goes under a timestamped directory.
# Per-file binaries and logs live in WORK_DIR — no shared paths,
# so parallel workers never collide.
OUTDIR="bench_results_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$OUTDIR"

REPORT_FILE="$OUTDIR/benchmark_report.txt"
SKIPPED_FILE="$OUTDIR/skipped_user_input.txt"
SKIPPED_NAME_FILE="$OUTDIR/skipped_name_filter.txt"
STATS_FILE="$OUTDIR/per_benchmark_stats.csv"
WORK_DIR="$OUTDIR/work"
mkdir -p "$WORK_DIR"

# ============================================================
# User-input patterns
# ============================================================
USER_INPUT_PATTERNS=(
    '\bcin\s*>>'
    '\bgetline\s*\('
    '\bscanf\s*\('
    '\bfscanf\s*\(stdin'
    '\bfgets\s*\('
    '\bgetchar\s*\('
    '\bgets\s*\('
    '\bread\s*\(\s*STDIN_FILENO'
    '\bgetc\s*\(\s*stdin'
    '\bfgetc\s*\(\s*stdin'
    'std::cin'
    '\bargc\b.*\bargv\b'
    '\bargv\s*\['
)

# ============================================================
# Pure helper functions (called from subshells — no global state)
# ============================================================

contains_large_numbers() {
    local output="$1"
    while IFS= read -r num; do
        [[ -z "$num" ]] && continue
        if (( num > LARGE_NUM_THRESHOLD || num < NEGATIVE_NUM_THRESHOLD )); then
            return 0
        fi
    done < <(echo "$output" | grep -oE '\-?[0-9]{6,}')
    return 1
}

classify_asan() {
    local asan_err="$1"
    if echo "$asan_err" | grep -qiE 'ERROR: AddressSanitizer'; then
        echo "ASAN_CAUGHT"
    elif echo "$asan_err" | grep -qiE 'WARNING: AddressSanitizer'; then
        echo "ASAN_WARNING"
    else
        echo "ASAN_CLEAN"
    fi
}

filename_is_valid() {
    local filename="$1"
    local base
    base=$(basename "$filename" .cpp)
    if ! echo "$base" | grep -qiE '(UAS|bug)'; then
        echo "name does not contain 'UAS' or 'bug'"
        return 1
    fi
    if echo "$base" | grep -qiE '(^|[^a-z])no([^a-z]|$)'; then
        echo "name contains 'no' — likely a non-buggy variant"
        return 1
    fi
    return 0
}

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
# Worker — runs entirely in a subshell.
# Writes results to per-file files under WORK_DIR only.
# No writes to any shared file → zero race conditions.
#
# Files written per benchmark (keyed by slug):
#   <slug>.log           full human-readable output
#   <slug>.verdict       single-word verdict
#   <slug>.csv           one CSV data row
#   <slug>.skipped_name  present if skipped by name filter
#   <slug>.skipped_input present if skipped by input scan
# ============================================================
analyse_benchmark() {
    local src="$1"
    local filename
    filename=$(basename "$src")

    # Unique slug: replace non-alphanum with _
    local slug
    slug=$(echo "$filename" | tr -cs 'a-zA-Z0-9' '_')

    local log_file="$WORK_DIR/${slug}.log"
    local verdict_file="$WORK_DIR/${slug}.verdict"
    local csv_file="$WORK_DIR/${slug}.csv"
    local binary="$WORK_DIR/${slug}.bin"   # unique binary path per worker

    flog() { echo -e "$*" >> "$log_file"; }
    fhr()  { flog "${CYAN}$(printf '─%.0s' {1..70})${NC}"; }

    fhr
    flog "${BOLD}▶  FILE: $filename${NC}"
    flog "   Path: $src"
    flog ""

    # --- Step 1: Filename filter ---
    local name_reason
    name_reason=$(filename_is_valid "$filename")
    if [[ $? -ne 0 ]]; then
        flog "  ${BLUE}[SKIPPED — NAME FILTER]${NC} $name_reason"
        flog ""
        echo "SKIPPED_NAME" > "$verdict_file"
        { echo "FILE: $src"; echo "  Reason: $name_reason"; echo ""; } \
            > "$WORK_DIR/${slug}.skipped_name"
        printf '"%s","%s",%d,0,0,0,0,"SKIPPED_NAME"\n' \
            "$filename" "$src" "$RUNS" > "$csv_file"
        return
    fi

    # --- Step 2: User-input scan ---
    local scan_result
    scan_result=$(takes_user_input "$src")
    if [[ $? -eq 0 ]]; then
        local matched_pattern matched_line
        matched_pattern="${scan_result%%|||*}"
        matched_line="${scan_result##*|||}"
        flog "  ${YELLOW}[SKIPPED — USER INPUT]${NC}"
        flog "  ${YELLOW}  Pattern : ${matched_pattern}${NC}"
        flog "  ${YELLOW}  At line : ${matched_line}${NC}"
        flog ""
        echo "SKIPPED_USER_INPUT" > "$verdict_file"
        { echo "FILE: $src"; echo "  Pattern : $matched_pattern"
          echo "  Line    : $matched_line"; echo ""; } \
            > "$WORK_DIR/${slug}.skipped_input"
        printf '"%s","%s",%d,0,0,0,0,"SKIPPED_USER_INPUT"\n' \
            "$filename" "$src" "$RUNS" > "$csv_file"
        return
    fi

    # --- Step 3: Compile ---
    local compile_output compile_rc
    compile_output=$("$CLANGPP" $ASAN_FLAGS $LINK_FLAGS "$src" -o "$binary" 2>&1)
    compile_rc=$?
    if [[ $compile_rc -ne 0 ]]; then
        flog "${RED}  [COMPILE ERROR]${NC}"
        flog "$compile_output"
        echo "COMPILE_ERROR" > "$verdict_file"
        printf '"%s","%s",%d,0,0,0,0,"COMPILE_ERROR"\n' \
            "$filename" "$src" "$RUNS" > "$csv_file"
        return
    fi
    flog "  ${GREEN}Compiled OK${NC}"

    # --- Step 4: Run RUNS valid (non-segfault) times ---
    local asan_caught=0 segfault_count=0 large_num_count=0 clean_count=0
    local asan_output_sample="" valid_runs=0 total_attempts=0
    local max_attempts=$(( RUNS * 10 ))

    while (( valid_runs < RUNS && total_attempts < max_attempts )); do
        local stdout_file stderr_file
        stdout_file=$(mktemp)
        stderr_file=$(mktemp)

        timeout 15 "$binary" >"$stdout_file" 2>"$stderr_file"
        local exit_code=$?
        ((total_attempts++))

        local stdout_content stderr_content
        stdout_content=$(cat "$stdout_file")
        stderr_content=$(cat "$stderr_file")
        rm -f "$stdout_file" "$stderr_file"

        # Detect segfault via exit code 139 or message in stderr
        if (( exit_code == 139 )) || \
           echo "$stderr_content" | grep -qiE 'Segmentation fault|SIGSEGV'; then
            ((segfault_count++))
            continue   # does not count toward valid_runs
        fi

        ((valid_runs++))

        local asan_class
        asan_class=$(classify_asan "$stderr_content")

        if [[ "$asan_class" == "ASAN_CAUGHT" || "$asan_class" == "ASAN_WARNING" ]]; then
            ((asan_caught++))
            [[ -z "$asan_output_sample" ]] && asan_output_sample="$stderr_content"
        elif contains_large_numbers "$stdout_content"; then
            ((large_num_count++))
            [[ -z "$asan_output_sample" ]] && asan_output_sample="$stdout_content"
        else
            ((clean_count++))
        fi
    done

    rm -f "$binary"

    # --- Verdict ---
    flog ""
    flog "  ${BOLD}Results — $valid_runs valid run(s) of $RUNS target ($total_attempts total attempts, $segfault_count segfaults):${NC}"
    flog "    ASan caught        : $asan_caught"
    flog "    Segfaults (skipped): $segfault_count"
    flog "    Large-num output   : $large_num_count  ${YELLOW}(ASan missed)${NC}"
    flog "    Clean runs         : $clean_count"
    flog ""

    local verdict
    if (( asan_caught > 0 )); then
        verdict="ASAN_CAUGHT"
        flog "  ${RED}${BOLD}[VERDICT] ❌ ASan DETECTED a bug${NC}"
        local asan_summary
        asan_summary=$(echo "$asan_output_sample" | grep -A 20 'ERROR: AddressSanitizer' | head -25)
        flog "${RED}  --- ASan Report (excerpt) ---${NC}"
        flog "$asan_summary"
    elif (( segfault_count > 0 && large_num_count == 0 )); then
        verdict="SEGFAULT_ONLY"
        flog "  ${RED}${BOLD}[VERDICT] 💥 SEGFAULT only — bug present, ASan silent${NC}"
    elif (( large_num_count > 0 && segfault_count > 0 )); then
        verdict="INTERMITTENT"
        flog "  ${YELLOW}${BOLD}[VERDICT] ⚠️  Intermittent — garbage output + segfaults${NC}"
        flog "${YELLOW}  --- Sample anomalous output ---${NC}"
        flog "$(echo "$asan_output_sample" | head -15)"
    elif (( large_num_count > 0 )); then
        verdict="ASAN_MISSED"
        flog "  ${YELLOW}${BOLD}[VERDICT] ⚠️  ASan MISSED — garbage values in output${NC}"
        flog "${YELLOW}  --- Sample anomalous output ---${NC}"
        flog "$(echo "$asan_output_sample" | head -15)"
    else
        verdict="CLEAN"
        flog "  ${GREEN}${BOLD}[VERDICT] ✅ No bug detected${NC}"
    fi

    echo "$verdict" > "$verdict_file"
    printf '"%s","%s",%d,%d,%d,%d,%d,"%s"\n' \
        "$filename" "$src" "$RUNS" \
        "$asan_caught" "$segfault_count" "$large_num_count" "$clean_count" \
        "$verdict" > "$csv_file"

    flog ""
}

# Export everything the background subshells need
export -f analyse_benchmark contains_large_numbers classify_asan \
           filename_is_valid takes_user_input
export CLANGPP ASAN_FLAGS LINK_FLAGS RUNS WORK_DIR OUTDIR
export LARGE_NUM_THRESHOLD NEGATIVE_NUM_THRESHOLD
export RED GREEN YELLOW BLUE CYAN BOLD NC
export USER_INPUT_PATTERNS

# ============================================================
# Main
# ============================================================

{ echo "========================================"
  echo " Files Skipped — Name Filter"
  echo " Generated: $(date)"
  echo "========================================"; echo ""; } > "$SKIPPED_NAME_FILE"

{ echo "========================================"
  echo " Files Skipped — Require User Input"
  echo " Generated: $(date)"
  echo "========================================"; echo ""; } > "$SKIPPED_FILE"

echo '"filename","path","total_runs","asan_caught_runs","segfault_runs","large_num_runs","clean_runs","verdict"' \
    > "$STATS_FILE"

echo ""
echo -e "${BOLD}${CYAN}╔══════════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BOLD}${CYAN}║   MTUASBench — ASan Benchmark Analysis Script (PARALLEL)         ║${NC}"
echo -e "${BOLD}${CYAN}╚══════════════════════════════════════════════════════════════════╝${NC}"
echo ""
echo "  Clang++    : $CLANGPP"
echo "  Bench dir  : $BENCH_DIR"
echo "  Runs/bench : $RUNS  (valid non-segfault runs)"
echo "  Parallel   : $MAX_JOBS workers  ($(nproc) CPU cores detected)"
echo "  Output dir : $OUTDIR"
echo ""

if ! command -v "$CLANGPP" &>/dev/null && [[ ! -x "$CLANGPP" ]]; then
    echo -e "${RED}ERROR: clang++ not found at '$CLANGPP'${NC}"; exit 1
fi

mapfile -t CPP_FILES < <(find "$BENCH_DIR" -name "*.cpp" | sort)

if [[ ${#CPP_FILES[@]} -eq 0 ]]; then
    echo -e "${RED}No .cpp files found in $BENCH_DIR${NC}"; exit 1
fi

echo "  Found ${#CPP_FILES[@]} file(s) — launching up to $MAX_JOBS in parallel..."
echo ""

# ---- Parallel dispatch with job-slot semaphore ----
active_jobs=0
total_files=${#CPP_FILES[@]}
done_files=0

for src in "${CPP_FILES[@]}"; do
    analyse_benchmark "$src" &
    ((active_jobs++))
    ((done_files++))
    printf "\r  Progress: %d / %d dispatched  (%d running)" \
        "$done_files" "$total_files" "$active_jobs"
    if (( active_jobs >= MAX_JOBS )); then
        wait -n 2>/dev/null || wait
        ((active_jobs--))
    fi
done
wait   # drain all remaining workers
echo ""
echo ""
echo "  All workers finished. Collecting results..."
echo ""

# ============================================================
# Collect — merge per-file outputs into shared report + CSV
# ============================================================

{
    echo ""
    echo -e "${BOLD}${CYAN}╔══════════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BOLD}${CYAN}║   MTUASBench — ASan Benchmark Analysis Script (PARALLEL)         ║${NC}"
    echo -e "${BOLD}${CYAN}╚══════════════════════════════════════════════════════════════════╝${NC}"
    echo ""
    echo "  Clang++  : $CLANGPP | Bench dir: $BENCH_DIR"
    echo "  Runs     : $RUNS    | Jobs: $MAX_JOBS"
    echo ""
} > "$REPORT_FILE"

# Merge individual logs (sorted by filename for deterministic order)
for log_file in $(ls "$WORK_DIR"/*.log 2>/dev/null | sort); do
    cat "$log_file" >> "$REPORT_FILE"
done

# Merge skipped lists
for f in "$WORK_DIR"/*.skipped_name;  do [[ -f "$f" ]] && cat "$f" >> "$SKIPPED_NAME_FILE"; done
for f in "$WORK_DIR"/*.skipped_input; do [[ -f "$f" ]] && cat "$f" >> "$SKIPPED_FILE";      done

# Merge CSV rows (sorted)
for f in $(ls "$WORK_DIR"/*.csv 2>/dev/null | sort); do
    cat "$f" >> "$STATS_FILE"
done

# ---- Summary table ----
echo -e "${CYAN}$(printf '─%.0s' {1..70})${NC}" | tee -a "$REPORT_FILE"
echo "" | tee -a "$REPORT_FILE"
echo -e "${BOLD}${CYAN}SUMMARY${NC}" | tee -a "$REPORT_FILE"
echo "" | tee -a "$REPORT_FILE"
printf "  %-60s %s\n" "Benchmark" "Verdict"         | tee -a "$REPORT_FILE"
printf "  %-60s %s\n" "$(printf '─%.0s' {1..60})" "$(printf '─%.0s' {1..20})" | tee -a "$REPORT_FILE"

asan_caught_total=0; asan_missed_total=0; clean_total=0
compile_err_total=0; segfault_total=0; intermittent_total=0
skipped_input_total=0; skipped_name_total=0

# Read CSV rows in sorted order to drive the table (skip header line)
while IFS=',' read -r fn _path _rest; do
    fn="${fn//\"/}"
    [[ "$fn" == "filename" ]] && continue

    slug=$(echo "$fn" | tr -cs 'a-zA-Z0-9' '_')
    verdict_file="$WORK_DIR/${slug}.verdict"
    [[ -f "$verdict_file" ]] || continue
    verdict=$(cat "$verdict_file")

    case "$verdict" in
        ASAN_CAUGHT)        color=$RED;    icon="❌ ASan caught";       ((asan_caught_total++))   ;;
        ASAN_MISSED)        color=$YELLOW; icon="⚠️  ASan missed";       ((asan_missed_total++))   ;;
        CLEAN)              color=$GREEN;  icon="✅ Clean";              ((clean_total++))         ;;
        COMPILE_ERROR)      color=$RED;    icon="🔨 Compile error";     ((compile_err_total++))   ;;
        SEGFAULT_ONLY)      color=$RED;    icon="💥 Segfault only";     ((segfault_total++))      ;;
        INTERMITTENT)       color=$YELLOW; icon="⚠️  Intermittent";      ((intermittent_total++))  ;;
        SKIPPED_USER_INPUT) color=$BLUE;   icon="⏭️  Skipped (input)";   ((skipped_input_total++)) ;;
        SKIPPED_NAME)       color=$BLUE;   icon="⏭️  Skipped (name)";    ((skipped_name_total++))  ;;
        *)                  color=$NC;     icon="?  Unknown"             ;;
    esac
    printf "  ${color}%-60s %s${NC}\n" "$fn" "$icon" | tee -a "$REPORT_FILE"
done < "$STATS_FILE"

# Aggregate stats
ran_total=$(( asan_caught_total + asan_missed_total + intermittent_total + segfault_total + clean_total + compile_err_total ))
bug_files=$(( asan_caught_total + asan_missed_total + intermittent_total + segfault_total ))
detection_rate=0; missed_rate=0
if (( bug_files > 0 )); then
    detection_rate=$(( asan_caught_total * 100 / bug_files ))
    missed_rate=$(( (asan_missed_total + intermittent_total + segfault_total) * 100 / bug_files ))
fi

{
    echo ""
    echo -e "  ${BOLD}── Per-Category Totals ──────────────────────────────${NC}"
    echo    "    ❌ ASan caught                : $asan_caught_total"
    echo    "    ⚠️  ASan missed (garbage out)  : $asan_missed_total"
    echo    "    💥 Segfault only              : $segfault_total"
    echo    "    ⚠️  Intermittent               : $intermittent_total"
    echo    "    ✅ Clean / no bug observed    : $clean_total"
    echo    "    🔨 Compile errors             : $compile_err_total"
    echo    "    ⏭️  Skipped (user input)       : $skipped_input_total"
    echo    "    ⏭️  Skipped (name filter)      : $skipped_name_total"
    echo ""
    echo -e "  ${BOLD}── Aggregate Detection Stats (over $ran_total analysed files) ──${NC}"
    echo    "    Bug files attempted          : $bug_files"
    echo    "    ASan detection rate          : ${detection_rate}%  ($asan_caught_total caught / $bug_files)"
    echo    "    ASan miss rate               : ${missed_rate}%  ($((asan_missed_total + intermittent_total + segfault_total)) missed / $bug_files)"
    echo ""
    echo -e "  ${BOLD}── Output Files ─────────────────────────────────────${NC}"
    echo    "    Full report     : $REPORT_FILE"
    echo    "    Per-bench CSV   : $STATS_FILE"
    echo    "    Skipped (input) : $SKIPPED_FILE"
    echo    "    Skipped (name)  : $SKIPPED_NAME_FILE"
    echo    "    Raw work dir    : $WORK_DIR"
    echo ""
} | tee -a "$REPORT_FILE"