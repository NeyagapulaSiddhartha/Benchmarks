#!/bin/bash

# ============================================================
# UAS Bug Count Analyzer
# Scans all .cpp files under ROOT_DIR whose names contain
# 'UAS' and extracts the =N suffix to tally bug counts.
#
# Usage: ./count_uas_bugs.sh <root_dir>
# ============================================================

ROOT_DIR="${1:-.}"

if [[ ! -d "$ROOT_DIR" ]]; then
    echo "ERROR: Directory '$ROOT_DIR' not found."
    exit 1
fi

echo ""
echo "======================================================================"
echo "  UAS Bug-Count Analyzer"
echo "======================================================================"
echo "  Root dir : $ROOT_DIR"
echo ""

# ---- Collect all .cpp files containing 'UAS' in their name ----
mapfile -t UAS_FILES < <(find "$ROOT_DIR" -name "*.cpp" | grep -E 'UAS' | sort)

if [[ ${#UAS_FILES[@]} -eq 0 ]]; then
    echo "  No .cpp files with 'UAS' in the name found under $ROOT_DIR"
    exit 0
fi

echo "  Found ${#UAS_FILES[@]} file(s) with 'UAS' in the name."
echo ""

# ---- Tally =N counts ----
declare -A COUNT_MAP   # N -> count of files
declare -a NO_SUFFIX   # files with UAS but no =N

for f in "${UAS_FILES[@]}"; do
    base=$(basename "$f" .cpp)

    # Extract =N — last occurrence of =<digits> in the filename
    if echo "$base" | grep -qE '=[0-9]+'; then
        n=$(echo "$base" | grep -oE '=[0-9]+' | tail -1 | tr -d '=')
        COUNT_MAP["$n"]=$(( ${COUNT_MAP["$n"]:-0} + 1 ))
    else
        NO_SUFFIX+=("$f")
    fi
done

# ---- Per-directory breakdown ----
echo "────────────────────────────────────────────────────────────────────────"
echo "  Per-Directory Breakdown"
echo "────────────────────────────────────────────────────────────────────────"
echo ""

# Collect unique directories that have UAS files
declare -A DIR_FILES   # rel_dir -> space-separated "base=N" entries

for f in "${UAS_FILES[@]}"; do
    dir=$(dirname "$f")
    rel_dir="${dir#$ROOT_DIR/}"
    [[ "$rel_dir" == "$dir" ]] && rel_dir="."
    base=$(basename "$f" .cpp)

    if echo "$base" | grep -qE '=[0-9]+'; then
        n=$(echo "$base" | grep -oE '=[0-9]+' | tail -1 | tr -d '=')
        DIR_FILES["$rel_dir"]+="$base|$n "
    else
        DIR_FILES["$rel_dir"]+="$base|NONE "
    fi
done

for rel_dir in $(echo "${!DIR_FILES[@]}" | tr ' ' '\n' | sort); do
    printf "  Directory: %s\n" "$rel_dir"
    IFS=' ' read -ra entries <<< "${DIR_FILES[$rel_dir]}"
    for entry in "${entries[@]}"; do
        [[ -z "$entry" ]] && continue
        fname="${entry%%|*}"
        n="${entry##*|}"
        if [[ "$n" == "NONE" ]]; then
            printf "    %-60s  (no =N suffix)\n" "${fname}.cpp"
        else
            printf "    %-60s  =%s bugs\n" "${fname}.cpp" "$n"
        fi
    done
    echo ""
done

# ---- Summary table ----
echo "────────────────────────────────────────────────────────────────────────"
echo "  Summary — UAS files by bug count"
echo "────────────────────────────────────────────────────────────────────────"
echo ""
printf "  %-12s  %s\n" "Bug Count" "Number of files"
printf "  %-12s  %s\n" "$(printf '─%.0s' {1..12})" "$(printf '─%.0s' {1..20})"

total=0
for n in $(echo "${!COUNT_MAP[@]}" | tr ' ' '\n' | sort -n); do
    cnt="${COUNT_MAP[$n]}"
    printf "  =%-11s  %d\n" "$n" "$cnt"
    (( total += cnt ))
done

if (( ${#NO_SUFFIX[@]} > 0 )); then
    printf "  %-12s  %d\n" "(no =N)" "${#NO_SUFFIX[@]}"
    (( total += ${#NO_SUFFIX[@]} ))
fi

echo ""
printf "  %-12s  %d\n" "TOTAL" "$total"
echo ""

# ---- Files with no =N suffix (if any) ----
if (( ${#NO_SUFFIX[@]} > 0 )); then
    echo "────────────────────────────────────────────────────────────────────────"
    echo "  UAS files with no =N suffix"
    echo "────────────────────────────────────────────────────────────────────────"
    for f in "${NO_SUFFIX[@]}"; do
        rel="${f#$ROOT_DIR/}"
        printf "  %s\n" "$rel"
    done
    echo ""
fi

echo "────────────────────────────────────────────────────────────────────────"
echo ""