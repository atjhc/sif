#!/usr/bin/env bash
set -euo pipefail

# Usage: scripts/perf.sh [build_type]
# Runs all Advent of Code problems and measures their execution times.
# Records results to perf/advent_times.csv for performance tracking.
#
# Arguments:
#   build_type  Optional. Either "debug" or "release" (default: release)

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
RESULT_DIR="$ROOT_DIR/perf"
RESULT_FILE="$RESULT_DIR/advent_times.csv"
ADVENT_DIR="$RESULT_DIR/advent"
BUILD_ROOT="$ROOT_DIR/build"

BUILD_TYPE="${1:-release}"
case "$BUILD_TYPE" in
  debug|Debug)
    BUILD_TYPE="debug"
    CMAKE_TYPE="Debug"
    ;;
  release|Release)
    BUILD_TYPE="release"
    CMAKE_TYPE="Release"
    ;;
  *)
    echo "Unknown build type: $BUILD_TYPE (use 'debug' or 'release')" >&2
    exit 1
    ;;
esac

BUILD_DIR="$BUILD_ROOT/$BUILD_TYPE"
SIF_TOOL="$BUILD_DIR/sif_tool"

mkdir -p "$RESULT_DIR"

if [[ ! -f "$RESULT_FILE" ]]; then
  printf 'timestamp,build_type,year,day,part,milliseconds\n' > "$RESULT_FILE"
fi

ensure_sdkroot() {
  if [[ -n "${SDKROOT:-}" ]]; then
    return
  fi
  if command -v xcrun >/dev/null 2>&1; then
    SDKROOT="$(xcrun --show-sdk-path 2>/dev/null || true)"
    export SDKROOT
  fi
}

build_sif() {
  echo "Building Sif ($CMAKE_TYPE)..."
  ensure_sdkroot
  cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$CMAKE_TYPE" >/dev/null
  cmake --build "$BUILD_DIR" >/dev/null
  echo "Build complete."
  echo
}

run_problem() {
  local problem_path="$1"
  local year day part

  # Extract year/day/part from path
  # Example: /path/to/advent/2024/01/part1.sif
  year=$(basename "$(dirname "$(dirname "$problem_path")")")
  day=$(basename "$(dirname "$problem_path")")
  part=$(basename "$problem_path" .sif)

  # Run the problem and measure time in milliseconds
  local start_ms end_ms duration_ms
  start_ms=$(perl -MTime::HiRes=time -e 'printf "%.0f", time()*1000')

  if ! (cd "$(dirname "$problem_path")" && "$SIF_TOOL" "$(basename "$problem_path")" >/dev/null 2>&1); then
    echo "  FAILED: Year $year Day $day $part" >&2
    return 1
  fi

  end_ms=$(perl -MTime::HiRes=time -e 'printf "%.0f", time()*1000')
  duration_ms=$((end_ms - start_ms))

  # Record result
  printf '%s,%s,%s,%s,%s,%d\n' \
    "$(date -Iseconds)" \
    "$CMAKE_TYPE" \
    "$year" \
    "$day" \
    "$part" \
    "$duration_ms" >> "$RESULT_FILE"

  printf "  Year %s Day %s %s: %dms\n" "$year" "$day" "$part" "$duration_ms"
  return 0
}

# Build first
build_sif

# Find all Advent of Code problems
echo "Running Advent of Code performance tests ($CMAKE_TYPE)..."
echo

total=0
passed=0
failed=0

while IFS= read -r -d '' problem; do
  ((total++)) || true
  if run_problem "$problem"; then
    ((passed++)) || true
  else
    ((failed++)) || true
  fi
done < <(find "$ADVENT_DIR" -name "*.sif" -type f -print0 | sort -z)

echo
echo "========================================="
echo "Performance test summary:"
echo "  Total:  $total problems"
echo "  Passed: $passed"
echo "  Failed: $failed"
echo "========================================="
echo
echo "Results appended to: $RESULT_FILE"

if [[ $failed -gt 0 ]]; then
  exit 1
fi
