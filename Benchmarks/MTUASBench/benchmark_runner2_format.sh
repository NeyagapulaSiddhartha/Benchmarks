#!/bin/bash

# ============================================================
# Benchmark Analysis Script — ASan + Output Anomaly Detection
#                             PARALLEL VERSION
# Usage: ./run_benchmarks.sh <clang++_path> <benchmark_dir> [runs] [jobs]
# ============================================================

CLANGPP="${1:-clang++}"
BENCH_DIR="${2:-.}"
RUNS="${3:-5}"
MAX_JOBS="${4:-$(nproc)}"

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

OUTDIR="bench_results_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$OUTDIR"

REPORT_FILE="$OUTDIR/benchmark_report.txt"
SKIPPED_FILE="$OUTDIR/skipped_user_input.txt"
SKIPPED_NAME_FILE="$OUTDIR/skipped_name_filter.txt"
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
# Helper functions
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
    # Source the patterns file — needed because bash cannot export arrays
    # and this function runs inside a subshell (background worker).
    # shellcheck source=/dev/null
    [[ -f "$PATTERNS_FILE" ]] && source "$PATTERNS_FILE"
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
# Worker — runs in a subshell, writes only to its own files
# ============================================================
analyse_benchmark() {
    local src="$1"
    local filename
    filename=$(basename "$src")

    local slug
    slug=$(echo "$filename" | tr -cs 'a-zA-Z0-9' '_')

    local log_file="$WORK_DIR/${slug}.log"
    local verdict_file="$WORK_DIR/${slug}.verdict"
    local binary="$WORK_DIR/${slug}.bin"

    # Local log — writes only to this file's log, never to shared files
    flog() { printf '%b\n' "$*" >> "$log_file"; }
    fhr()  { flog "──────────────────────────────────────────────────────────────────────"; }

    fhr
    flog "FILE: $filename"
    flog "Path: $src"
    flog ""

    # --- Step 1: Filename filter ---
    local name_reason
    name_reason=$(filename_is_valid "$filename")
    if [[ $? -ne 0 ]]; then
        flog "  [SKIPPED — NAME FILTER] $name_reason"
        flog ""
        echo "SKIPPED_NAME" > "$verdict_file"
        { echo "FILE: $src"; echo "  Reason: $name_reason"; echo ""; } \
            > "$WORK_DIR/${slug}.skipped_name"
        return
    fi

    # --- Step 2: User-input scan ---
    local scan_result
    scan_result=$(takes_user_input "$src")
    if [[ $? -eq 0 ]]; then
        local matched_pattern matched_line
        matched_pattern="${scan_result%%|||*}"
        matched_line="${scan_result##*|||}"
        flog "  [SKIPPED — USER INPUT]"
        flog "    Pattern : ${matched_pattern}"
        flog "    At line : ${matched_line}"
        flog ""
        echo "SKIPPED_USER_INPUT" > "$verdict_file"
        { echo "FILE: $src"; echo "  Pattern : $matched_pattern"
          echo "  Line    : $matched_line"; echo ""; } \
            > "$WORK_DIR/${slug}.skipped_input"
        return
    fi

    # --- Step 3: Compile ---
    local compile_output compile_rc
    compile_output=$("$CLANGPP" $ASAN_FLAGS $LINK_FLAGS "$src" -o "$binary" 2>&1)
    compile_rc=$?
    if [[ $compile_rc -ne 0 ]]; then
        flog "  [COMPILE ERROR]"
        flog "$compile_output"
        flog ""
        echo "COMPILE_ERROR" > "$verdict_file"
        return
    fi
    flog "  Compiled OK"

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

        if (( exit_code == 139 )) || \
           echo "$stderr_content" | grep -qiE 'Segmentation fault|SIGSEGV'; then
            ((segfault_count++))
            continue
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

    # --- Results block ---
    flog ""
    flog "  Results over $valid_runs run(s) ($total_attempts total attempts, $segfault_count segfaults):"
    flog "    ASan error caught  : $asan_caught"
    flog "    Segfaults          : $segfault_count"
    flog "    Large-number output: $large_num_count  (ASan missed, UAS visible in output)"
    flog "    Clean runs         : $clean_count"

    # --- Verdict ---
    local verdict
    if (( asan_caught > 0 )); then
        verdict="ASAN_CAUGHT"
        flog ""
        flog "  [VERDICT] FAIL — ASan DETECTED a bug"
        flog ""
        local asan_summary
        asan_summary=$(echo "$asan_output_sample" \
            | grep -A 20 'ERROR: AddressSanitizer' | head -25)
        flog "  --- ASan Report (excerpt) ---"
        flog "$asan_summary"

    elif (( segfault_count > 0 && large_num_count == 0 )); then
        verdict="SEGFAULT_ONLY"
        flog ""
        flog "  [VERDICT] FAIL — SEGFAULT detected, ASan silent"

    elif (( large_num_count > 0 && segfault_count > 0 )); then
        verdict="INTERMITTENT"
        flog ""
        flog "  [VERDICT] FAIL — Intermittent (garbage output + segfaults, ASan missed)"
        flog ""
        flog "  --- Sample anomalous output ---"
        flog "$(echo "$asan_output_sample" | head -15)"

    elif (( large_num_count > 0 )); then
        verdict="ASAN_MISSED"
        flog ""
        flog "  [VERDICT] FAIL — ASan MISSED the bug (garbage values in output)"
        flog ""
        flog "  --- Sample anomalous output ---"
        flog "$(echo "$asan_output_sample" | head -15)"

    else
        verdict="CLEAN"
        flog ""
        flog "  [VERDICT] PASS — Program appears to run correctly (no errors detected)"
    fi

    echo "$verdict" > "$verdict_file"

    # Save raw counts for per-directory aggregation (tab-separated)
    # format: src_path  valid_runs  asan_caught  segfault_count  large_num_count  clean_count  verdict
    printf '%s\t%d\t%d\t%d\t%d\t%d\t%s\n' \
        "$src" "$valid_runs" "$asan_caught" \
        "$segfault_count" "$large_num_count" "$clean_count" \
        "$verdict" > "$WORK_DIR/${slug}.stats"

    flog ""
}

# Bash cannot export arrays across subshell boundaries via `export`.
# Write the patterns to a file that each worker sources at startup.
PATTERNS_FILE="$WORK_DIR/input_patterns.sh"
{
    echo "USER_INPUT_PATTERNS=("
    for p in "${USER_INPUT_PATTERNS[@]}"; do
        printf "    %q\n" "$p"
    done
    echo ")"
} > "$PATTERNS_FILE"

export -f analyse_benchmark contains_large_numbers classify_asan \
           filename_is_valid takes_user_input
export CLANGPP ASAN_FLAGS LINK_FLAGS RUNS WORK_DIR OUTDIR PATTERNS_FILE
export LARGE_NUM_THRESHOLD NEGATIVE_NUM_THRESHOLD
export RED GREEN YELLOW BLUE CYAN BOLD NC

# ============================================================
# Main
# ============================================================

{ echo "Files Skipped — Name Filter"
  echo "Generated: $(date)"; echo ""; } > "$SKIPPED_NAME_FILE"

{ echo "Files Skipped — Require User Input"
  echo "Generated: $(date)"; echo ""; } > "$SKIPPED_FILE"

echo ""
echo "======================================================================"
echo "       MTUASBench — ASan Benchmark Analysis Script (PARALLEL)"
echo "======================================================================"
echo ""
echo "  Clang++    : $CLANGPP"
echo "  Bench dir  : $BENCH_DIR"
echo "  Runs/bench : $RUNS  (valid non-segfault runs)"
echo "  Parallel   : $MAX_JOBS workers  ($(nproc) CPU cores detected)"
echo "  Output dir : $OUTDIR"
echo ""

if ! command -v "$CLANGPP" &>/dev/null && [[ ! -x "$CLANGPP" ]]; then
    echo "ERROR: clang++ not found at '$CLANGPP'"; exit 1
fi

mapfile -t CPP_FILES < <(find "$BENCH_DIR" -name "*.cpp" | sort)

if [[ ${#CPP_FILES[@]} -eq 0 ]]; then
    echo "No .cpp files found in $BENCH_DIR"; exit 1
fi

echo "  Found ${#CPP_FILES[@]} file(s) — launching up to $MAX_JOBS in parallel..."
echo ""

# Parallel dispatch
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
wait
echo ""
echo ""
echo "  All workers finished. Collecting results..."
echo ""

# ============================================================
# Collect — merge per-file logs, build report
# ============================================================

{
    echo ""
    echo "======================================================================"
    echo "       MTUASBench — ASan Benchmark Analysis Script (PARALLEL)"
    echo "======================================================================"
    echo ""
    echo "  Clang++  : $CLANGPP"
    echo "  Bench dir: $BENCH_DIR"
    echo "  Runs     : $RUNS  |  Jobs: $MAX_JOBS"
    echo ""
} > "$REPORT_FILE"

# Merge individual logs in sorted order
for log_file in $(ls "$WORK_DIR"/*.log 2>/dev/null | sort); do
    cat "$log_file" >> "$REPORT_FILE"
done

# Merge skipped lists
for f in "$WORK_DIR"/*.skipped_name;  do [[ -f "$f" ]] && cat "$f" >> "$SKIPPED_NAME_FILE"; done
for f in "$WORK_DIR"/*.skipped_input; do [[ -f "$f" ]] && cat "$f" >> "$SKIPPED_FILE";      done

# ---- Tally verdicts ----
asan_caught_total=0; asan_missed_total=0; clean_total=0
compile_err_total=0; segfault_total=0; intermittent_total=0
skipped_input_total=0; skipped_name_total=0

declare -A FILE_VERDICT   # filename -> verdict

for verdict_file in $(ls "$WORK_DIR"/*.verdict 2>/dev/null | sort); do
    slug=$(basename "$verdict_file" .verdict)
    verdict=$(cat "$verdict_file")
    # Recover original filename from log first line
    log_file="$WORK_DIR/${slug}.log"
    fn=$(grep '^FILE:' "$log_file" 2>/dev/null | head -1 | sed 's/^FILE: //')
    [[ -z "$fn" ]] && fn="$slug"
    FILE_VERDICT["$fn"]="$verdict"

    case "$verdict" in
        ASAN_CAUGHT)        ((asan_caught_total++))   ;;
        ASAN_MISSED)        ((asan_missed_total++))   ;;
        CLEAN)              ((clean_total++))         ;;
        COMPILE_ERROR)      ((compile_err_total++))   ;;
        SEGFAULT_ONLY)      ((segfault_total++))      ;;
        INTERMITTENT)       ((intermittent_total++))  ;;
        SKIPPED_USER_INPUT) ((skipped_input_total++)) ;;
        SKIPPED_NAME)       ((skipped_name_total++))  ;;
    esac
done

# ---- Summary table (also written to report) ----
{
    echo "────────────────────────────────────────────────────────────────────────"
    echo ""
    echo "SUMMARY"
    echo ""
    printf "  %-60s %s\n" "Benchmark" "Verdict"
    printf "  %-60s %s\n" "$(printf '─%.0s' {1..60})" "$(printf '─%.0s' {1..20})"
    echo ""
} | tee -a "$REPORT_FILE"

for fn in $(echo "${!FILE_VERDICT[@]}" | tr ' ' '\n' | sort); do
    verdict="${FILE_VERDICT[$fn]}"
    case "$verdict" in
        ASAN_CAUGHT)        icon="FAIL  — ASan caught"       ;;
        ASAN_MISSED)        icon="FAIL  — ASan missed"       ;;
        CLEAN)              icon="PASS  — Clean"             ;;
        COMPILE_ERROR)      icon="ERROR — Compile error"     ;;
        SEGFAULT_ONLY)      icon="FAIL  — Segfault only"     ;;
        INTERMITTENT)       icon="FAIL  — Intermittent"      ;;
        SKIPPED_USER_INPUT) icon="SKIP  — User input"        ;;
        SKIPPED_NAME)       icon="SKIP  — Name filter"       ;;
        *)                  icon="?     — Unknown"           ;;
    esac
    printf "  %-60s %s\n" "$fn" "$icon" | tee -a "$REPORT_FILE"
done

# Aggregate stats
ran_total=$(( asan_caught_total + asan_missed_total + intermittent_total \
              + segfault_total + clean_total + compile_err_total ))
bug_files=$(( asan_caught_total + asan_missed_total + intermittent_total + segfault_total ))
detection_rate=0; missed_rate=0
if (( bug_files > 0 )); then
    detection_rate=$(( asan_caught_total * 100 / bug_files ))
    missed_rate=$(( (asan_missed_total + intermittent_total + segfault_total) * 100 / bug_files ))
fi

{
    echo ""
    echo "────────────────────────────────────────────────────────────────────────"
    echo "Per-Category Totals"
    echo "────────────────────────────────────────────────────────────────────────"
    echo "    ASan caught                : $asan_caught_total"
    echo "    ASan missed (garbage out)  : $asan_missed_total"
    echo "    Segfault only              : $segfault_total"
    echo "    Intermittent               : $intermittent_total"
    echo "    Clean / no bug observed    : $clean_total"
    echo "    Compile errors             : $compile_err_total"
    echo "    Skipped (user input)       : $skipped_input_total"
    echo "    Skipped (name filter)      : $skipped_name_total"
    echo ""
    echo "────────────────────────────────────────────────────────────────────────"
    echo "Aggregate Detection Stats  (over $ran_total analysed files)"
    echo "────────────────────────────────────────────────────────────────────────"
    echo "    Bug files attempted        : $bug_files"
    echo "    ASan detection rate        : ${detection_rate}%  ($asan_caught_total caught / $bug_files)"
    echo "    ASan miss rate             : ${missed_rate}%  ($((asan_missed_total + intermittent_total + segfault_total)) missed / $bug_files)"
    echo ""
} | tee -a "$REPORT_FILE"

# ============================================================
# Per-directory aggregation
# For every unique parent directory, collect all .stats files
# whose path starts with that directory, then compute:
#   - total valid runs across all benchmarks in the dir
#   - total ASan-caught runs  → catch rate per 100 runs
#   - average catch rate across benchmarks (mean of per-file rates)
# ============================================================
{
    echo "────────────────────────────────────────────────────────────────────────"
    echo "Per-Directory Aggregation  (catch rate per $RUNS runs)"
    echo "────────────────────────────────────────────────────────────────────────"
    echo ""
} | tee -a "$REPORT_FILE"

# Collect all directories that have at least one .stats file
declare -A DIR_SEEN
for stats_file in $(ls "$WORK_DIR"/*.stats 2>/dev/null | sort); do
    src_path=$(awk -F'\t' '{print $1}' "$stats_file")
    dir=$(dirname "$src_path")
    # Relativise for display
    rel_dir="${dir#$BENCH_DIR/}"
    [[ "$rel_dir" == "$dir" ]] && rel_dir="$dir"   # fallback if not under BENCH_DIR
    DIR_SEEN["$rel_dir"]="$dir"
done

# Sort directories and print one block each
for rel_dir in $(echo "${!DIR_SEEN[@]}" | tr ' ' '\n' | sort); do
    abs_dir="${DIR_SEEN[$rel_dir]}"

    dir_total_runs=0
    dir_caught_runs=0
    dir_valid_files=0
    dir_catch_rate_sum=0   # sum of per-file catch rates (for averaging)

    # Collect per-file stats for files in this directory
    declare -A per_file_data  # filename -> "asan_caught/valid_runs"

    for stats_file in $(ls "$WORK_DIR"/*.stats 2>/dev/null | sort); do
        src_path=$(awk    -F'\t' '{print $1}' "$stats_file")
        valid_runs=$(awk  -F'\t' '{print $2}' "$stats_file")
        asan_caught=$(awk -F'\t' '{print $3}' "$stats_file")
        verdict=$(awk     -F'\t' '{print $7}' "$stats_file")

        # Only count files in this directory that were actually run
        [[ "$(dirname "$src_path")" != "$abs_dir" ]] && continue
        [[ "$verdict" == SKIPPED_* || "$verdict" == "COMPILE_ERROR" ]] && continue
        (( valid_runs == 0 )) && continue

        fn=$(basename "$src_path")
        per_file_data["$fn"]="$asan_caught/$valid_runs"

        dir_total_runs=$(( dir_total_runs + valid_runs ))
        dir_caught_runs=$(( dir_caught_runs + asan_caught ))
        (( dir_valid_files++ ))

        # Per-file catch rate * 100 stored as integer (e.g. 73 = 73%)
        file_rate=$(( asan_caught * 100 / valid_runs ))
        dir_catch_rate_sum=$(( dir_catch_rate_sum + file_rate ))
    done

    (( dir_valid_files == 0 )) && continue

    avg_catch_rate=$(( dir_catch_rate_sum / dir_valid_files ))
    overall_catch_rate=0
    (( dir_total_runs > 0 )) && \
        overall_catch_rate=$(( dir_caught_runs * 100 / dir_total_runs ))

    {
        printf "  Directory : %s\n" "$rel_dir"
        printf "  Files run : %d   |   Total valid runs : %d\n" \
            "$dir_valid_files" "$dir_total_runs"
        echo ""
        printf "    %-50s  %s\n" "Benchmark" "Catch rate"
        printf "    %-50s  %s\n" "$(printf '─%.0s' {1..50})" "$(printf '─%.0s' {1..12})"
        for fn in $(echo "${!per_file_data[@]}" | tr ' ' '\n' | sort); do
            caught="${per_file_data[$fn]%%/*}"
            runs="${per_file_data[$fn]##*/}"
            rate=$(( caught * 100 / runs ))
            printf "    %-50s  %d / %d runs  (%d%%)\n" "$fn" "$caught" "$runs" "$rate"
        done
        echo ""
        printf "    %-50s  %d%%\n" "Average catch rate (mean of files above)" "$avg_catch_rate"
        printf "    %-50s  %d / %d  (%d%%)\n" \
            "Overall catch rate (pooled across all runs)" \
            "$dir_caught_runs" "$dir_total_runs" "$overall_catch_rate"
        echo ""
        echo "────────────────────────────────────────────────────────────────────────"
        echo ""
    } | tee -a "$REPORT_FILE"

    unset per_file_data
    declare -A per_file_data
done

{
    echo "────────────────────────────────────────────────────────────────────────"
    echo "Output Files"
    echo "────────────────────────────────────────────────────────────────────────"
    echo "    Full report     : $REPORT_FILE"
    if (( skipped_input_total > 0 )); then
    echo "    Skipped (input) : $SKIPPED_FILE"
    fi
    if (( skipped_name_total > 0 )); then
    echo "    Skipped (name)  : $SKIPPED_NAME_FILE"
    fi
    echo ""
} | tee -a "$REPORT_FILE"

sed -i 's/\x1B\[[0-9;]*[mKHF]//g' "$REPORT_FILE" "$SKIPPED_FILE" "$SKIPPED_NAME_FILE"
