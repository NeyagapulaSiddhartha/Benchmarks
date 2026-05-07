#!/bin/bash

# ============================================================
# Benchmark Analysis Script — ASan + Output Anomaly Detection
#                             PARALLEL VERSION
#   Merged: Excel + timing + per-bin (old) + thread tracking (v2)
#
#   Features:
#     - Per-run ASan bug detection with bin breakdown
#     - Execution timing (valid runs only)
#     - Thread-aware bug detection: identifies which thread
#       triggered ASan, tallies per-thread fault counts
#     - Stats file has 10 fields (field 10 = thread_str)
#     - Text report: per-thread breakdown per benchmark
#     - Per-directory and paper-category thread summaries
#     - Excel: "Fault Threads" column (soft-yellow highlight)
#
# Usage: ./run_benchmarks_merged.sh <clang++_path> <benchmark_dir> [runs] [jobs]
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

# Make sure llvm-symbolizer is found so ASan prints line numbers
# CLANG_BIN_DIR="$(dirname "$(command -v "$CLANGPP" 2>/dev/null || echo "$CLANGPP")")"
# if [[ -x "$CLANG_BIN_DIR/llvm-symbolizer" ]]; then
#     export PATH="$CLANG_BIN_DIR:$PATH"
#     export ASAN_SYMBOLIZER_PATH="$CLANG_BIN_DIR/llvm-symbolizer"
# fi


export ASAN_OPTIONS="symbolize=1"

# ============================================================
# Check Python + openpyxl availability
# ============================================================
PYTHON_OK=0
if command -v python3 &>/dev/null; then
    if python3 -c "import openpyxl" 2>/dev/null; then
        PYTHON_OK=1
    else
        echo "  WARNING: openpyxl not found. Excel output will be skipped."
        echo "           Install with: pip install openpyxl --break-system-packages"
    fi
else
    echo "  WARNING: python3 not found. Excel output will be skipped."
fi

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

# ============================================================
# Extract the faulting thread ID from one ASan stderr dump.
# Looks for the first line like:
#   READ of size N at 0x... thread T2
#   WRITE of size N at 0x... thread T2
# Falls back to the thread mentioned in the ERROR: line if needed.
# Returns empty string if not found.
# ============================================================
# extract_fault_thread() {
#     local asan_err="$1"

#     # Primary: READ/WRITE line carries the faulting thread
#     local tid
#     tid=$(echo "$asan_err" \
#         | grep -oE '(READ|WRITE) of size [0-9]+ at 0x[0-9a-f]+ thread (T[0-9]+)' \
#         | head -1 \
#         | grep -oE 'T[0-9]+$')

#     # Fallback: SEGV / unknown-crash line
#     if [[ -z "$tid" ]]; then
#         tid=$(echo "$asan_err" \
#             | grep -oE '\(pc 0x[0-9a-f]+ bp 0x[0-9a-f]+ sp 0x[0-9a-f]+ T[0-9]+\)' \
#             | head -1 \
#             | grep -oE 'T[0-9]+\)$' \
#             | tr -d ')')
#     fi

#     # Fallback: stack-overflow line
#     if [[ -z "$tid" ]]; then
#         tid=$(echo "$asan_err" \
#             | grep -oE 'on address 0x[0-9a-f]+ \(pc.*T[0-9]+\)' \
#             | head -1 \
#             | grep -oE 'T[0-9]+\)$' \
#             | tr -d ')')
#     fi

#     echo "$tid"
# }
extract_fault_thread() {
    local asan_err="$1"
    local dyn_tid=""

    # ============================================================
    # LEVEL 1: READ/WRITE style reports
    # "READ of size 4 at 0x... thread T3"
    # ============================================================
    dyn_tid=$(echo "$asan_err" \
        | grep -oE '(READ|WRITE) of size [0-9]+ at 0x[0-9a-f]+ thread T[0-9]+' \
        | head -1 \
        | grep -oE 'T[0-9]+$')

    # ============================================================
    # LEVEL 2: SEGV / DEADLYSIGNAL line
    # "ERROR: AddressSanitizer: SEGV ... T5)"
    # "AddressSanitizer:DEADLYSIGNAL ... T0)"
    # ============================================================
    if [[ -z "$dyn_tid" ]]; then
        dyn_tid=$(echo "$asan_err" \
            | grep -oE 'AddressSanitizer:?(DEADLYSIGNAL|SEGV|stack-overflow)[^)]*T[0-9]+\)' \
            | head -1 \
            | grep -oE 'T[0-9]+\)' \
            | tr -d ')')
    fi

    # ============================================================
    # LEVEL 3: Generic pc/bp/sp line fallback
    # "(pc 0x... bp 0x... sp 0x... T5)"
    # ============================================================
    if [[ -z "$dyn_tid" ]]; then
        dyn_tid=$(echo "$asan_err" \
            | grep -oE '\(pc 0x[0-9a-f]+[^)]*T[0-9]+\)' \
            | head -1 \
            | grep -oE 'T[0-9]+\)' \
            | tr -d ')')
    fi

    # ============================================================
    # SPECIAL CASE: error types that never carry a thread ID
    # Handle BEFORE the "no dyn_tid → UNKNOWN" bail-out
    # ============================================================
    if [[ -z "$dyn_tid" ]]; then
        # "Joining already joined thread, aborting."
        if echo "$asan_err" | grep -qiE 'joining already joined thread'; then
            echo "DOUBLE_JOIN"
            return
        fi
        # "data race" with no explicit thread line
        if echo "$asan_err" | grep -qiE 'data race'; then
            echo "DATA_RACE"
            return
        fi
        # truly nothing found
        echo "UNKNOWN"
        return
    fi

    # ============================================================
    # LEVEL 4: Try to resolve dyn_tid → static pthread_create@LN
    # Works when llvm-symbolizer produces .cpp:line:col output.
    # When symbolization is broken (zlib error, missing symbolizer)
    # there will be no .cpp lines in the creation block — fall
    # through to Level 5 instead of returning UNKNOWN.
    # ============================================================
    local creation_block
    creation_block=$(echo "$asan_err" \
        | sed -n "/Thread ${dyn_tid} created by/,/^Thread T[0-9]\+ created by/p")

    # ============================================================
# LEVEL 4:
# Resolve dyn_tid -> pthread_create call site
#
# We specifically want the FIRST user frame AFTER:
# __interceptor_pthread_create
#
# Example:
#
# Thread T1 created by T0 here:
#   #0 ... __interceptor_pthread_create ...
#   #1 ... createThread() ... foo.cpp:51:5
#
# We want:
#   foo.cpp:51:5
# ============================================================

    local loc

    loc=$(echo "$asan_err" | awk -v tid="$dyn_tid" '

        # --------------------------------------------------------
        # Find correct thread creation block
        # --------------------------------------------------------
        $0 ~ ("Thread " tid " created by") {
            in_block=1
            next
        }

        # --------------------------------------------------------
        # Stop if next thread block starts
        # --------------------------------------------------------
        in_block && /^Thread T[0-9]+ created by/ {
            exit
        }

        # --------------------------------------------------------
        # Skip interceptor frame
        # --------------------------------------------------------
        in_block && /__interceptor_pthread_create/ {
            seen_interceptor=1
            next
        }

        # --------------------------------------------------------
        # First real user frame after interceptor
        # --------------------------------------------------------
        in_block && seen_interceptor && /\.cpp:[0-9]+:[0-9]+/ {

            match($0, /[^ ]+\.cpp:[0-9]+:[0-9]+/)

            print substr($0, RSTART, RLENGTH)

            exit
        }
    ')

    if [[ -n "$loc" ]]; then

        local line

        line=$(echo "$loc" \
            | grep -oE ':[0-9]+:' \
            | head -1 \
            | tr -d ':')

        echo "${dyn_tid}@L${line}"
        return
    fi

    # ============================================================
    # LEVEL 5: Symbolization failed (zlib, missing symbolizer, T0)
    # Return the raw dynamic TID — still useful for tallying which
    # thread faulted, even without a source location.
    # ============================================================
    echo "$dyn_tid"
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
    [[ -f "$PATTERNS_FILE" ]] && source "$PATTERNS_FILE"

    local stripped
    if command -v gcc &>/dev/null; then
        stripped=$(gcc -fpreprocess-only -w -x c++ - < "$src" 2>/dev/null)
        [[ -z "$stripped" ]] && \
            stripped=$(sed 's|/\*.*\*/||g; s|//.*||' "$src" 2>/dev/null)
    else
        stripped=$(sed 's|/\*.*\*/||g; s|//.*||' "$src" 2>/dev/null)
    fi

    for pattern in "${USER_INPUT_PATTERNS[@]}"; do
        local match
        match=$(echo "$stripped" | grep -nE "$pattern" | head -1)
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
    local DOWORK_SRC="/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.cpp"
    local extra_src=""
    [[ -f "$DOWORK_SRC" ]] && extra_src="$DOWORK_SRC"

    compile_output=$("$CLANGPP" $ASAN_FLAGS $LINK_FLAGS "$src" $extra_src -o "$binary" 2>&1)
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
    local exec_time_total=0

    # Per-valid-run result array: "CAUGHT", "LARGE", or "CLEAN"
    declare -a run_results=()

    # ---- Thread tracking: thread_id -> catch count ----
    declare -A thread_catch_map

    while (( valid_runs < RUNS && total_attempts < max_attempts )); do
        local stdout_file stderr_file
        stdout_file=$(mktemp)
        stderr_file=$(mktemp)

        local t_start t_end elapsed
        t_start=$(date +%s%N)
        timeout 15 "$binary" >"$stdout_file" 2>"$stderr_file"
        local exit_code=$?
        t_end=$(date +%s%N)
        elapsed=$(( (t_end - t_start) ))   # nanoseconds
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
        exec_time_total=$(( exec_time_total + elapsed ))

        local asan_class
        asan_class=$(classify_asan "$stderr_content")

        # AFTER:
if [[ "$asan_class" == "ASAN_CAUGHT" || "$asan_class" == "ASAN_WARNING" ]]; then

    local fault_tid
    fault_tid=$(extract_fault_thread "$stderr_content")

    # --------------------------------------------------------
    # If we could not identify any thread ID, the ASan report
    # is incomplete/unresolvable. Do NOT count it as a catch.
    # Treat it like a segfault: bump segfault_count so the run
    # slot stays open and we retry up to max_attempts.
    # Dump the output for debugging just like before.
    # --------------------------------------------------------
    if [[ -z "$fault_tid" || "$fault_tid" == "UNKNOWN" || "$fault_tid" == "DOUBLE_JOIN" || "$fault_tid" == "DATA_RACE" ]] || \
       echo "$fault_tid" | grep -qE '^T[0-9]+$'; then
        ((segfault_count++))
        # still dump for debugging
        mkdir -p "$OUTDIR/unknown_thread_reports"
        unknown_file="$OUTDIR/unknown_thread_reports/${slug}_run${valid_runs}.txt"
        {
            echo "======================================================"
            echo "FILE      : $filename"
            echo "ATTEMPT   : $total_attempts"
            echo "TIMESTAMP : $(date)"
            echo "NOTE      : ASan fired but thread ID unresolvable"
            echo "           — treated as invalid run (not counted)"
            echo "======================================================"
            echo ""
            echo "FULL ASAN OUTPUT:"
            echo ""
            echo "$stderr_content"
            echo ""
        } > "$unknown_file"
        # This run does not count — undo the valid_runs increment
        # that happened before the ASan classification block.
        # We incremented valid_runs and exec_time_total already,
        # so roll them back.
        ((valid_runs--))
        exec_time_total=$(( exec_time_total - elapsed ))
        continue
    fi

    # Normal path — thread ID is known
    ((asan_caught++))
    run_results+=("CAUGHT")
    [[ -z "$asan_output_sample" ]] && asan_output_sample="$stderr_content"
    thread_catch_map["$fault_tid"]=$(( ${thread_catch_map["$fault_tid"]:-0} + 1 ))
    
        elif contains_large_numbers "$stdout_content"; then
            ((large_num_count++))
            run_results+=("LARGE")
            [[ -z "$asan_output_sample" ]] && asan_output_sample="$stdout_content"
        else
            ((clean_count++))
            run_results+=("CLEAN")
        fi
    done

    rm -f "$binary"

    # Convert nanoseconds to seconds with 3 decimal places
    local exec_time_sec
    exec_time_sec=$(awk "BEGIN {printf \"%.3f\", $exec_time_total / 1000000000}")

    # --- Build bin stats ---
    local BIN_SIZE=10
    local num_bins=$(( (valid_runs + BIN_SIZE - 1) / BIN_SIZE ))
    local bin_str=""
    local bin_display=""

    for (( b=0; b<num_bins; b++ )); do
        local bin_start=$(( b * BIN_SIZE ))
        local bin_end=$(( bin_start + BIN_SIZE ))
        (( bin_end > valid_runs )) && bin_end=$valid_runs
        local bin_caught=0
        local bin_total=$(( bin_end - bin_start ))
        for (( r=bin_start; r<bin_end; r++ )); do
            [[ "${run_results[$r]}" == "CAUGHT" ]] && ((bin_caught++))
        done
        local label_start=$(( bin_start + 1 ))
        local label_end=$bin_end
        bin_display+=$(printf "    Runs %3d-%3d : %2d / %2d caught\n" \
            "$label_start" "$label_end" "$bin_caught" "$bin_total")
        [[ -n "$bin_str" ]] && bin_str+=","
        bin_str+="${bin_caught}"
    done

    # --- Build thread breakdown string e.g. "T1:4,T2:2" ---
    local thread_str=""
    local unique_thread_count=0
    if (( ${#thread_catch_map[@]} > 0 )); then
        local sorted_tids
        sorted_tids=$(
            for tid in "${!thread_catch_map[@]}"; do
                echo "$tid"
            done | sort -t'T' -k2 -n
        )
        for tid in $sorted_tids; do
            [[ -n "$thread_str" ]] && thread_str+=","
            thread_str+="${tid}:${thread_catch_map[$tid]}"
            (( unique_thread_count++ ))
        done
    else
        thread_str="none"
    fi

    # --- Results block ---
    flog ""
    flog "  Results over $valid_runs run(s) ($total_attempts total attempts, $segfault_count segfaults):"
    flog "    ASan error caught  : $asan_caught"
    flog "    Segfaults          : $segfault_count"
    flog "    Large-number output: $large_num_count  (ASan missed, UAS visible in output)"
    flog "    Clean runs         : $clean_count"
    flog "    Execution time     : ${exec_time_sec}s  (valid runs only)"
    flog ""

    # --- Thread breakdown block ---
    if (( asan_caught > 0 )); then
        flog "  Thread fault breakdown ($asan_caught ASan catches across $unique_thread_count unique thread(s)):"
        for tid in $(echo "${!thread_catch_map[@]}" | tr ' ' '\n' | sort -t'T' -k2 -n); do
            local cnt="${thread_catch_map[$tid]}"
            local pct
            pct=$(awk "BEGIN {printf \"%d\", $cnt * 100 / $asan_caught}")
            flog "    ${tid}  :  ${cnt} catch(es)  (${pct}% of all ASan catches)"
        done
        flog ""
    fi

    flog "  Per-bin breakdown (${BIN_SIZE} valid runs per bin):"
    flog "$bin_display"

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

    # Save stats — 10 tab-separated fields:
    # src  valid_runs  asan_caught  segfault_count  large_num_count
    # clean_count  verdict  exec_time_sec  bin_str  thread_str
    printf '%s\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%s\t%s\n' \
        "$src" "$valid_runs" "$asan_caught" \
        "$segfault_count" "$large_num_count" "$clean_count" \
        "$verdict" "$exec_time_sec" "$bin_str" "$thread_str" \
        > "$WORK_DIR/${slug}.stats"

    flog ""
}

# Export patterns file for subshells
PATTERNS_FILE="$WORK_DIR/input_patterns.sh"
{
    echo "USER_INPUT_PATTERNS=("
    for p in "${USER_INPUT_PATTERNS[@]}"; do
        printf "    %q\n" "$p"
    done
    echo ")"
} > "$PATTERNS_FILE"

export -f analyse_benchmark contains_large_numbers classify_asan \
           filename_is_valid takes_user_input extract_fault_thread
export CLANGPP ASAN_FLAGS LINK_FLAGS RUNS WORK_DIR OUTDIR PATTERNS_FILE
export DOWORK_SRC="/home/sidda/FINDUS_Artifact/FINDUS/Benchmarks/MTUASBench/Utilites/doWork.cpp"
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
    echo "  Clang++  : $CLANGPP"c
    echo "  Bench dir: $BENCH_DIR"
    echo "  Runs     : $RUNS  |  Jobs: $MAX_JOBS"
    echo ""
} > "$REPORT_FILE"

for log_file in $(ls "$WORK_DIR"/*.log 2>/dev/null | sort); do
    cat "$log_file" >> "$REPORT_FILE"
done

for f in "$WORK_DIR"/*.skipped_name;  do [[ -f "$f" ]] && cat "$f" >> "$SKIPPED_NAME_FILE"; done
for f in "$WORK_DIR"/*.skipped_input; do [[ -f "$f" ]] && cat "$f" >> "$SKIPPED_FILE";      done

# ---- Tally verdicts ----
asan_caught_total=0; asan_missed_total=0; clean_total=0
compile_err_total=0; segfault_total=0; intermittent_total=0
skipped_input_total=0; skipped_name_total=0

declare -A FILE_VERDICT

for verdict_file in $(ls "$WORK_DIR"/*.verdict 2>/dev/null | sort); do
    slug=$(basename "$verdict_file" .verdict)
    verdict=$(cat "$verdict_file")
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

# ---- Summary table ----
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
# Per-directory aggregation (includes thread info)
# ============================================================
{
    echo "────────────────────────────────────────────────────────────────────────"
    echo "Per-Directory Aggregation  (catch rate per $RUNS runs)"
    echo "────────────────────────────────────────────────────────────────────────"
    echo ""
} | tee -a "$REPORT_FILE"

declare -A DIR_SEEN
for stats_file in $(ls "$WORK_DIR"/*.stats 2>/dev/null | sort); do
    src_path=$(awk -F'\t' '{print $1}' "$stats_file")
    dir=$(dirname "$src_path")
    rel_dir="${dir#$BENCH_DIR/}"
    [[ "$rel_dir" == "$dir" ]] && rel_dir="$dir"
    DIR_SEEN["$rel_dir"]="$dir"
done

for rel_dir in $(echo "${!DIR_SEEN[@]}" | tr ' ' '\n' | sort); do
    abs_dir="${DIR_SEEN[$rel_dir]}"

    dir_total_runs=0
    dir_caught_runs=0
    dir_valid_files=0
    dir_catch_rate_sum=0

    declare -A dir_thread_map
    declare -A per_file_data
    declare -A per_file_threads

    for stats_file in $(ls "$WORK_DIR"/*.stats 2>/dev/null | sort); do
        src_path=$(awk    -F'\t' '{print $1}' "$stats_file")
        valid_runs=$(awk  -F'\t' '{print $2}' "$stats_file")
        asan_caught=$(awk -F'\t' '{print $3}' "$stats_file")
        verdict=$(awk     -F'\t' '{print $7}' "$stats_file")
        thread_str=$(awk  -F'\t' '{print $10}' "$stats_file")

        [[ "$(dirname "$src_path")" != "$abs_dir" ]] && continue
        [[ "$verdict" == SKIPPED_* || "$verdict" == "COMPILE_ERROR" ]] && continue
        (( valid_runs == 0 )) && continue

        fn=$(basename "$src_path")
        per_file_data["$fn"]="$asan_caught/$valid_runs"
        per_file_threads["$fn"]="$thread_str"

        dir_total_runs=$(( dir_total_runs + valid_runs ))
        dir_caught_runs=$(( dir_caught_runs + asan_caught ))
        (( dir_valid_files++ ))

        file_rate=$(( asan_caught * 100 / valid_runs ))
        dir_catch_rate_sum=$(( dir_catch_rate_sum + file_rate ))

        # Merge thread counts into directory-level map
        if [[ "$thread_str" != "none" && -n "$thread_str" ]]; then
            IFS=',' read -ra tid_entries <<< "$thread_str"
            for entry in "${tid_entries[@]}"; do
                local_tid="${entry%%:*}"
                local_cnt="${entry##*:}"
                dir_thread_map["$local_tid"]=$(( ${dir_thread_map["$local_tid"]:-0} + local_cnt ))
            done
        fi
    done

    (( dir_valid_files == 0 )) && {
        unset dir_thread_map per_file_data per_file_threads
        declare -A dir_thread_map per_file_data per_file_threads
        continue
    }

    avg_catch_rate=$(( dir_catch_rate_sum / dir_valid_files ))
    overall_catch_rate=0
    (( dir_total_runs > 0 )) && \
        overall_catch_rate=$(( dir_caught_runs * 100 / dir_total_runs ))

    # Build directory thread string
    local_dir_thread_str=""
    for tid in $(echo "${!dir_thread_map[@]}" | tr ' ' '\n' | sort -t'T' -k2 -n); do
        [[ -n "$local_dir_thread_str" ]] && local_dir_thread_str+=", "
        local_dir_thread_str+="${tid}:${dir_thread_map[$tid]}"
    done
    [[ -z "$local_dir_thread_str" ]] && local_dir_thread_str="none"

    {
        printf "  Directory : %s\n" "$rel_dir"
        printf "  Files run : %d   |   Total valid runs : %d\n" \
            "$dir_valid_files" "$dir_total_runs"
        echo ""
        printf "    %-50s  %-14s  %s\n" "Benchmark" "Catch rate" "Fault threads"
        printf "    %-50s  %-14s  %s\n" \
            "$(printf '─%.0s' {1..50})" \
            "$(printf '─%.0s' {1..14})" \
            "$(printf '─%.0s' {1..20})"
        for fn in $(echo "${!per_file_data[@]}" | tr ' ' '\n' | sort); do
            caught="${per_file_data[$fn]%%/*}"
            runs="${per_file_data[$fn]##*/}"
            rate=$(( caught * 100 / runs ))
            tstr="${per_file_threads[$fn]}"
            [[ "$tstr" == "none" || -z "$tstr" ]] && tstr="-"
            printf "    %-50s  %d / %d  (%d%%)  %s\n" \
                "$fn" "$caught" "$runs" "$rate" "$tstr"
        done
        echo ""
        printf "    %-50s  %d%%\n" \
            "Average catch rate (mean of files above)" "$avg_catch_rate"
        printf "    %-50s  %d / %d  (%d%%)\n" \
            "Overall catch rate (pooled across all runs)" \
            "$dir_caught_runs" "$dir_total_runs" "$overall_catch_rate"
        printf "    %-50s  %s\n" \
            "Fault threads (directory total)" "$local_dir_thread_str"
        echo ""
        echo "────────────────────────────────────────────────────────────────────────"
        echo ""
    } | tee -a "$REPORT_FILE"

    unset dir_thread_map per_file_data per_file_threads
    declare -A dir_thread_map per_file_data per_file_threads
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

# ============================================================
# Paper-Category Summary Table (includes thread info)
# ============================================================
PAPER_CATEGORIES=(
    "Object Patterns|/Object_patterns"
    "Object Usage Across Threads|/Object_uses_across_threads"
    "Indirect Sharing|/Sharing_via_global_ptr"
    "Threads Created in Multiple Scopes|/Thread_creation_multiple_scopes"
    "Context Sensitivity|/Context_sentitivity"
    "Thread Handle Type|/Thread_handle_patterns"
    "Nested Thread Calls|/Nested_thread_call"
    "Multiple Joins|/Multiple_joins"
    "Threads Created in a Loop|/Thread_in_loop"
    "Join Inside Function|/creation_join_using_func"
    "Nested Function Calls|/Nested_func_call"
    "Inheritance Cases|/Inheritance_with_data"
)

{
    echo "════════════════════════════════════════════════════════════════════════"
    echo "Paper-Category Summary Table"
    echo "════════════════════════════════════════════════════════════════════════"
    echo ""
    printf "  %-38s  %8s  %8s  %8s  %12s  %s\n" \
        "Category" "Buggy" "Caught≥1" "Never" "Avg Catch%" "Fault Threads"
    printf "  %-38s  %8s  %8s  %8s  %12s  %s\n" \
        "$(printf '─%.0s' {1..38})" \
        "$(printf '─%.0s' {1..8})" \
        "$(printf '─%.0s' {1..8})" \
        "$(printf '─%.0s' {1..8})" \
        "$(printf '─%.0s' {1..12})" \
        "$(printf '─%.0s' {1..20})"
    echo ""
} | tee -a "$REPORT_FILE"

grand_buggy=0; grand_caught=0; grand_never=0; grand_rate_sum=0; grand_cat_count=0
declare -A grand_thread_map

for cat_entry in "${PAPER_CATEGORIES[@]}"; do
    cat_name="${cat_entry%%|*}"
    dir_fragments_str="${cat_entry##*|}"

    cat_buggy=0; cat_caught_ge1=0; cat_never=0
    cat_rate_sum=0
    declare -A cat_thread_map

    for stats_file in $(ls "$WORK_DIR"/*.stats 2>/dev/null | sort); do
        src_path=$(awk   -F'\t' '{print $1}' "$stats_file")
        valid_runs=$(awk -F'\t' '{print $2}' "$stats_file")
        asan_caught=$(awk -F'\t' '{print $3}' "$stats_file")
        verdict=$(awk    -F'\t' '{print $7}' "$stats_file")
        thread_str=$(awk -F'\t' '{print $10}' "$stats_file")
        fn=$(basename "$src_path")

        matched_dir=0
        IFS=',' read -ra frags <<< "$dir_fragments_str"
        for frag in "${frags[@]}"; do
            if [[ "$src_path" == *"$frag"* ]]; then
                matched_dir=1; break
            fi
        done
        (( matched_dir == 0 )) && continue

        [[ "$verdict" == SKIPPED_* || "$verdict" == "COMPILE_ERROR" ]] && continue
        if ! echo "$fn" | grep -qiE '(bug|UAS)'; then continue; fi
        (( valid_runs == 0 )) && continue

        (( cat_buggy++ ))
        file_rate=$(( asan_caught * 100 / valid_runs ))
        cat_rate_sum=$(( cat_rate_sum + file_rate ))

        if (( asan_caught > 0 )); then
            (( cat_caught_ge1++ ))
        else
            (( cat_never++ ))
        fi

        # Merge thread counts into category map
        if [[ "$thread_str" != "none" && -n "$thread_str" ]]; then
            IFS=',' read -ra tid_entries <<< "$thread_str"
            for entry in "${tid_entries[@]}"; do
                local_tid="${entry%%:*}"
                local_cnt="${entry##*:}"
                cat_thread_map["$local_tid"]=$(( ${cat_thread_map["$local_tid"]:-0} + local_cnt ))
                grand_thread_map["$local_tid"]=$(( ${grand_thread_map["$local_tid"]:-0} + local_cnt ))
            done
        fi
    done

    avg_rate=0
    (( cat_buggy > 0 )) && avg_rate=$(( cat_rate_sum / cat_buggy ))

    # Build category thread string
    cat_thread_str=""
    for tid in $(echo "${!cat_thread_map[@]}" | tr ' ' '\n' | sort -t'T' -k2 -n); do
        [[ -n "$cat_thread_str" ]] && cat_thread_str+=", "
        cat_thread_str+="${tid}:${cat_thread_map[$tid]}"
    done
    [[ -z "$cat_thread_str" ]] && cat_thread_str="—"

    printf "  %-38s  %8d  %8d  %8d  %11d%%  %s\n" \
        "$cat_name" "$cat_buggy" "$cat_caught_ge1" "$cat_never" "$avg_rate" \
        "$cat_thread_str" \
        | tee -a "$REPORT_FILE"

    (( grand_buggy    += cat_buggy    ))
    (( grand_caught   += cat_caught_ge1 ))
    (( grand_never    += cat_never    ))
    (( grand_rate_sum += cat_rate_sum ))
    (( grand_cat_count++ ))

    unset cat_thread_map
    declare -A cat_thread_map
done

grand_avg=0
(( grand_buggy > 0 )) && grand_avg=$(( grand_rate_sum / grand_buggy ))

# Build grand thread string
grand_thread_str=""
for tid in $(echo "${!grand_thread_map[@]}" | tr ' ' '\n' | sort -t'T' -k2 -n); do
    [[ -n "$grand_thread_str" ]] && grand_thread_str+=", "
    grand_thread_str+="${tid}:${grand_thread_map[$tid]}"
done
[[ -z "$grand_thread_str" ]] && grand_thread_str="—"

{
    echo ""
    printf "  %-38s  %8s  %8s  %8s  %12s  %s\n" \
        "$(printf '─%.0s' {1..38})" \
        "$(printf '─%.0s' {1..8})" \
        "$(printf '─%.0s' {1..8})" \
        "$(printf '─%.0s' {1..8})" \
        "$(printf '─%.0s' {1..12})" \
        "$(printf '─%.0s' {1..20})"
    printf "  %-38s  %8d  %8d  %8d  %11d%%  %s\n" \
        "TOTAL / OVERALL" "$grand_buggy" "$grand_caught" "$grand_never" \
        "$grand_avg" "$grand_thread_str"
    echo ""
    echo "  Columns:"
    echo "    Buggy        = bug files run (name contains 'bug' or 'UAS', not skipped)"
    echo "    Caught≥1     = files where ASan fired at least once across all runs"
    echo "    Never        = files where ASan never fired (missed every run)"
    echo "    Avg Catch%   = mean of per-file catch rates within the category"
    echo "    Fault Threads= TID:count pairs — which thread triggered ASan and how often"
    echo "                   e.g. T1:30, T2:12 means T1 was the faulting thread 30 times"
    echo "                   and T2 was the faulting thread 12 times"
    echo ""
    echo "════════════════════════════════════════════════════════════════════════"
    echo ""
} | tee -a "$REPORT_FILE"

sed -i 's/\x1B\[[0-9;]*[mKHF]//g' "$REPORT_FILE" "$SKIPPED_FILE" "$SKIPPED_NAME_FILE"

# ============================================================
# Excel Generation (includes Fault Threads column + timing + bins)
# ============================================================
if (( PYTHON_OK == 1 )); then
    EXCEL_FILE="$OUTDIR/benchmark_results.xlsx"
    NUM_BINS=$(( RUNS / 10 ))

    python3 - <<PYEOF
import openpyxl
from openpyxl.styles import Font, PatternFill, Alignment, Border, Side
from openpyxl.utils import get_column_letter

WORK_DIR   = "$WORK_DIR"
EXCEL_FILE = "$EXCEL_FILE"
RUNS       = $RUNS
NUM_BINS   = $NUM_BINS
BENCH_DIR  = "$BENCH_DIR"

PAPER_CATEGORIES = [
    ("Object Patterns",                    ["/Object_patterns"]),
    ("Object Usage Across Threads",        ["/Object_uses_across_threads"]),
    ("Indirect Sharing",                   ["/Sharing_via_global_ptr"]),
    ("Threads Created in Multiple Scopes", ["/Thread_creation_multiple_scopes"]),
    ("Context Sensitivity",                ["/Context_sentitivity"]),
    ("Thread Handle Type",                 ["/Thread_handle_patterns"]),
    ("Nested Thread Calls",                ["/Nested_thread_call"]),
    ("Multiple Joins",                     ["/Multiple_joins"]),
    ("Threads Created in a Loop",          ["/Thread_in_loop"]),
    ("Join Inside Function",               ["/creation_join_using_func"]),
    ("Nested Function Calls",              ["/Nested_func_call"]),
    ("Inheritance Cases",                  ["/Inheritance_with_data"]),
]

import os, glob

# Load all stats files — 10 fields, field index 9 = thread_str
stats = []
for sf in sorted(glob.glob(os.path.join(WORK_DIR, "*.stats"))):
    with open(sf) as f:
        line = f.read().strip()
    if not line:
        continue
    parts = line.split("\t")
    if len(parts) < 9:
        continue
    src_path    = parts[0]
    valid_runs  = int(parts[1])
    asan_caught = int(parts[2])
    verdict     = parts[6]
    exec_time   = parts[7]
    bin_str     = parts[8]
    thread_str  = parts[9] if len(parts) > 9 else "none"
    bins = [int(x) for x in bin_str.split(",") if x.strip().lstrip('-').isdigit()] if bin_str else []
    stats.append({
        "src":         src_path,
        "fn":          os.path.basename(src_path),
        "valid_runs":  valid_runs,
        "asan_caught": asan_caught,
        "verdict":     verdict,
        "exec_time":   exec_time,
        "bins":        bins,
        "thread_str":  thread_str if thread_str != "none" else "",
    })

# Style helpers
HDR_FILL    = PatternFill("solid", start_color="1F4E79")
BIN_FILL    = PatternFill("solid", start_color="D6E4F0")
TOTAL_FILL  = PatternFill("solid", start_color="E2EFDA")
THREAD_FILL = PatternFill("solid", start_color="FFF2CC")  # soft yellow for thread col
SKIP_FILL   = PatternFill("solid", start_color="F2F2F2")
HDR_FONT    = Font(name="Arial", bold=True, color="FFFFFF", size=10)
BODY_FONT   = Font(name="Arial", size=10)
BOLD_FONT   = Font(name="Arial", bold=True, size=10)
CENTER      = Alignment(horizontal="center", vertical="center", wrap_text=True)
LEFT        = Alignment(horizontal="left",   vertical="center", wrap_text=True)
thin        = Side(style="thin", color="BFBFBF")
BORDER      = Border(left=thin, right=thin, top=thin, bottom=thin)

def style_cell(cell, fill=None, font=None, align=None):
    if fill:  cell.fill  = fill
    if font:  cell.font  = font
    if align: cell.alignment = align
    cell.border = BORDER

wb = openpyxl.Workbook()
wb.remove(wb.active)

for cat_name, dir_fragments in PAPER_CATEGORIES:
    rows = []
    for s in stats:
        if s["verdict"].startswith("SKIPPED_") or s["verdict"] == "COMPILE_ERROR":
            continue
        if any(frag in s["src"] for frag in dir_fragments):
            rows.append(s)

    if not rows:
        continue

    sheet_name = cat_name[:31]
    ws = wb.create_sheet(title=sheet_name)

    # ---- Header row ----
    # Columns: Benchmark | Bin1..BinN | Total Caught | Total Runs | Catch% | Exec Time | Fault Threads
    headers = ["Benchmark"]
    for b in range(1, NUM_BINS + 1):
        start = (b - 1) * 10 + 1
        end   = b * 10
        headers.append(f"Bin {b}\n({start}-{end})")
    headers += ["Total\nCaught", "Total\nRuns", "Catch %", "Exec Time (s)", "Fault Threads\n(TID:count)"]

    for col, h in enumerate(headers, 1):
        cell = ws.cell(row=1, column=col, value=h)
        style_cell(cell, fill=HDR_FILL, font=HDR_FONT, align=CENTER)

    ws.row_dimensions[1].height = 32

    # ---- Data rows ----
    for row_idx, s in enumerate(sorted(rows, key=lambda x: x["fn"]), 2):
        is_skipped = s["verdict"].startswith("SKIPPED_")

        # Benchmark name
        cell = ws.cell(row=row_idx, column=1, value=s["fn"])
        style_cell(cell, fill=SKIP_FILL if is_skipped else None, font=BODY_FONT, align=LEFT)

        # Bin columns
        for b in range(NUM_BINS):
            col = b + 2
            val = "-" if (is_skipped or b >= len(s["bins"])) else s["bins"][b]
            cell = ws.cell(row=row_idx, column=col, value=val)
            style_cell(cell, fill=BIN_FILL if not is_skipped else SKIP_FILL,
                       font=BODY_FONT, align=CENTER)

        col_total = NUM_BINS + 2
        col_runs  = NUM_BINS + 3
        col_pct   = NUM_BINS + 4
        col_time  = NUM_BINS + 5
        col_thr   = NUM_BINS + 6   # Fault Threads column

        # Total Caught
        cell = ws.cell(row=row_idx, column=col_total,
                       value=s["asan_caught"] if not is_skipped else "-")
        style_cell(cell, fill=TOTAL_FILL if not is_skipped else SKIP_FILL,
                   font=BOLD_FONT, align=CENTER)

        # Total Runs
        cell = ws.cell(row=row_idx, column=col_runs,
                       value=s["valid_runs"] if not is_skipped else "-")
        style_cell(cell, fill=TOTAL_FILL if not is_skipped else SKIP_FILL,
                   font=BODY_FONT, align=CENTER)

        # Catch % — live Excel formula
        # Catch % — per-line breakdown e.g. "L56:40%, L34:20%"
        if not is_skipped and s["valid_runs"] > 0:
            thr_str = s["thread_str"]
            if thr_str:
                # Aggregate counts by line number
                line_counts = {}
                for entry in thr_str.split(","):
                    entry = entry.strip()
                    if not entry:
                        continue
                    # entry is like "T1@L35:2" or "T2@L35:3"
                    tid_part, _, cnt_part = entry.rpartition(":")
                    try:
                        cnt = int(cnt_part)
                        # Extract line number from @LN suffix
                        if "@L" in tid_part:
                            line_label = "L" + tid_part.split("@L")[1]
                        else:
                            line_label = tid_part  # fallback: use raw tid
                        line_counts[line_label] = line_counts.get(line_label, 0) + cnt
                    except ValueError:
                        pass

                # Build display string sorted by line number
                parts = []
                for line_label in sorted(line_counts.keys(),
                                         key=lambda x: int(x[1:]) if x[1:].isdigit() else 0):
                    pct = round(line_counts[line_label] * 100 / s["valid_runs"])
                    parts.append(f"{line_label}:{pct}%")
                pct_val = ", ".join(parts)
            else:
                pct_val = "0%"
        else:
            pct_val = "0%"
        cell = ws.cell(row=row_idx, column=col_pct, value=pct_val)
        style_cell(cell, fill=TOTAL_FILL if not is_skipped else SKIP_FILL,
                   font=BOLD_FONT, align=LEFT)

        # Exec time
        try:
            tval = float(s["exec_time"]) if not is_skipped else "-"
        except Exception:
            tval = "-"
        cell = ws.cell(row=row_idx, column=col_time, value=tval)
        if isinstance(tval, float):
            cell.number_format = "0.000"
        style_cell(cell, fill=None, font=BODY_FONT, align=CENTER)

        # Fault Threads (e.g. "T1:4,T2:2" or "-")
        thr_val = s["thread_str"] if (not is_skipped and s["thread_str"]) else "-"
        cell = ws.cell(row=row_idx, column=col_thr, value=thr_val)
        style_cell(cell,
                   fill=THREAD_FILL if (not is_skipped and thr_val != "-") else SKIP_FILL,
                   font=BODY_FONT, align=LEFT)

    # ---- Column widths ----
    ws.column_dimensions["A"].width = 52
    for b in range(1, NUM_BINS + 1):
        ws.column_dimensions[get_column_letter(b + 1)].width = 11
    ws.column_dimensions[get_column_letter(NUM_BINS + 2)].width = 10
    ws.column_dimensions[get_column_letter(NUM_BINS + 3)].width = 10
    ws.column_dimensions[get_column_letter(NUM_BINS + 4)].width = 10
    ws.column_dimensions[get_column_letter(NUM_BINS + 5)].width = 14
    ws.column_dimensions[get_column_letter(NUM_BINS + 6)].width = 24  # Fault Threads

    ws.freeze_panes = "A2"

# ============================================================
# Extra sheets — grouped by UAS bug count (UAS=N in filename)
# ============================================================
import re
from collections import defaultdict

# Extract UAS=N value from filename, return int or None
def get_uas_count(fn):
    m = re.search(r'UAS=(\d+)', fn, re.IGNORECASE)
    if m:
        return int(m.group(1))
    return None

# Build mapping: uas_count -> list of stat dicts
uas_groups = defaultdict(list)
for s in stats:
    if s["verdict"].startswith("SKIPPED_") or s["verdict"] == "COMPILE_ERROR":
        continue
    n = get_uas_count(s["fn"])
    if n is not None:
        uas_groups[n].append(s)

# One sheet per UAS count, sorted numerically
for uas_n in sorted(uas_groups.keys()):
    rows = sorted(uas_groups[uas_n], key=lambda x: x["fn"])
    if not rows:
        continue

    sheet_name = f"UAS={uas_n} Bug{'s' if uas_n != 1 else ''}"[:31]
    ws = wb.create_sheet(title=sheet_name)

    # ---- Header row (identical to category sheets) ----
    headers = ["Benchmark"]
    for b in range(1, NUM_BINS + 1):
        start = (b - 1) * 10 + 1
        end   = b * 10
        headers.append(f"Bin {b}\n({start}-{end})")
    headers += ["Total\nCaught", "Total\nRuns", "Catch %\n(per thread)", "Exec Time (s)", "Fault Threads\n(TID:count)"]

    for col, h in enumerate(headers, 1):
        cell = ws.cell(row=1, column=col, value=h)
        style_cell(cell, fill=HDR_FILL, font=HDR_FONT, align=CENTER)
    ws.row_dimensions[1].height = 32

    # ---- Data rows ----
    total_caught_sum = 0
    total_runs_sum   = 0

    for row_idx, s in enumerate(rows, 2):
        is_skipped = s["verdict"].startswith("SKIPPED_")

        # Benchmark name
        cell = ws.cell(row=row_idx, column=1, value=s["fn"])
        style_cell(cell, fill=SKIP_FILL if is_skipped else None, font=BODY_FONT, align=LEFT)

        # Bin columns
        for b in range(NUM_BINS):
            col = b + 2
            val = "-" if (is_skipped or b >= len(s["bins"])) else s["bins"][b]
            cell = ws.cell(row=row_idx, column=col, value=val)
            style_cell(cell, fill=BIN_FILL if not is_skipped else SKIP_FILL,
                       font=BODY_FONT, align=CENTER)

        col_total = NUM_BINS + 2
        col_runs  = NUM_BINS + 3
        col_pct   = NUM_BINS + 4
        col_time  = NUM_BINS + 5
        col_thr   = NUM_BINS + 6

        # Total Caught
        cell = ws.cell(row=row_idx, column=col_total,
                       value=s["asan_caught"] if not is_skipped else "-")
        style_cell(cell, fill=TOTAL_FILL if not is_skipped else SKIP_FILL,
                   font=BOLD_FONT, align=CENTER)

        # Total Runs
        cell = ws.cell(row=row_idx, column=col_runs,
                       value=s["valid_runs"] if not is_skipped else "-")
        style_cell(cell, fill=TOTAL_FILL if not is_skipped else SKIP_FILL,
                   font=BODY_FONT, align=CENTER)

# Catch % — per-thread breakdown e.g. "T10:20%, T11:40%"
        if not is_skipped and s["valid_runs"] > 0:
            thr_str = s["thread_str"]
            if thr_str:
                parts = []
                for entry in thr_str.split(","):
                    entry = entry.strip()
                    if not entry:
                        continue
                    # entry is like "T1@L35:2" or "T1:2"
                    # tid is everything before the last colon, cnt is after
                    tid_part, _, cnt_part = entry.rpartition(":")
                    try:
                        cnt = int(cnt_part)
                        pct = round(cnt * 100 / s["valid_runs"])
                        # Simplify tid display: strip @LN suffix
                        tid_display = tid_part.split("@")[0]
                        parts.append(f"{tid_display}:{pct}%")
                    except ValueError:
                        parts.append(entry)
                pct_val = ", ".join(parts)
            else:
                pct_val = "0%"
        else:
            pct_val = "0%"
        cell = ws.cell(row=row_idx, column=col_pct, value=pct_val)
        style_cell(cell, fill=TOTAL_FILL if not is_skipped else SKIP_FILL,
                   font=BOLD_FONT, align=LEFT)

        # Exec time
        try:
            tval = float(s["exec_time"]) if not is_skipped else "-"
        except Exception:
            tval = "-"
        cell = ws.cell(row=row_idx, column=col_time, value=tval)
        if isinstance(tval, float):
            cell.number_format = "0.000"
        style_cell(cell, fill=None, font=BODY_FONT, align=CENTER)

        # Fault Threads
        thr_val = s["thread_str"] if (not is_skipped and s["thread_str"]) else "-"
        cell = ws.cell(row=row_idx, column=col_thr, value=thr_val)
        style_cell(cell,
                   fill=THREAD_FILL if (not is_skipped and thr_val != "-") else SKIP_FILL,
                   font=BODY_FONT, align=LEFT)

        if not is_skipped:
            total_caught_sum += s["asan_caught"]
            total_runs_sum   += s["valid_runs"]

    # ---- Summary / totals row ----
    summary_row = len(rows) + 2
    SUMMARY_FILL = PatternFill("solid", start_color="D9D9D9")
    SUMMARY_FONT = Font(name="Arial", bold=True, size=10)

    cell = ws.cell(row=summary_row, column=1, value=f"TOTAL  ({len(rows)} file(s))")
    style_cell(cell, fill=SUMMARY_FILL, font=SUMMARY_FONT, align=LEFT)

    # Blank bin cells in summary row
    for b in range(NUM_BINS):
        cell = ws.cell(row=summary_row, column=b + 2, value="")
        style_cell(cell, fill=SUMMARY_FILL, font=SUMMARY_FONT, align=CENTER)

    col_total = NUM_BINS + 2
    col_runs  = NUM_BINS + 3
    col_pct   = NUM_BINS + 4
    col_time  = NUM_BINS + 5
    col_thr   = NUM_BINS + 6

    cell = ws.cell(row=summary_row, column=col_total, value=total_caught_sum)
    style_cell(cell, fill=SUMMARY_FILL, font=SUMMARY_FONT, align=CENTER)

    cell = ws.cell(row=summary_row, column=col_runs, value=total_runs_sum)
    style_cell(cell, fill=SUMMARY_FILL, font=SUMMARY_FONT, align=CENTER)

    # Overall catch % for this UAS group
    if total_runs_sum > 0:
        caught_col = get_column_letter(col_total)
        runs_col   = get_column_letter(col_runs)
        cell = ws.cell(row=summary_row, column=col_pct,
                       value=f"=IF({runs_col}{summary_row}>0,{caught_col}{summary_row}/{runs_col}{summary_row},0)")
        cell.number_format = "0.0%"
    else:
        cell = ws.cell(row=summary_row, column=col_pct, value="-")
    style_cell(cell, fill=SUMMARY_FILL, font=SUMMARY_FONT, align=CENTER)

    # Blank exec time + fault threads in summary row
    for col in (col_time, col_thr):
        cell = ws.cell(row=summary_row, column=col, value="")
        style_cell(cell, fill=SUMMARY_FILL, font=SUMMARY_FONT, align=CENTER)

    # ---- Column widths (same as category sheets) ----
    ws.column_dimensions["A"].width = 52
    for b in range(1, NUM_BINS + 1):
        ws.column_dimensions[get_column_letter(b + 1)].width = 11
    ws.column_dimensions[get_column_letter(NUM_BINS + 2)].width = 10
    ws.column_dimensions[get_column_letter(NUM_BINS + 3)].width = 10
    ws.column_dimensions[get_column_letter(NUM_BINS + 4)].width = 10
    ws.column_dimensions[get_column_letter(NUM_BINS + 5)].width = 14
    ws.column_dimensions[get_column_letter(NUM_BINS + 6)].width = 24

    ws.freeze_panes = "A2"

    print(f"  Sheet added: {sheet_name}  ({len(rows)} files)")

wb.save(EXCEL_FILE)
print(f"Excel saved: {EXCEL_FILE}")
PYEOF

    echo "  Excel report    : $EXCEL_FILE"
fi
