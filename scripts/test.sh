#!/usr/bin/env bash
set -euo pipefail

# Usage: scripts/test.sh
# Runs unit tests in Debug and Release configurations, capturing wall clock time
# for each ctest run and appending the measurements to perf/test_times.csv.

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
RESULT_DIR="$ROOT_DIR/perf"
RESULT_FILE="$RESULT_DIR/test_times.csv"
BUILD_ROOT="$ROOT_DIR/build"

mkdir -p "$RESULT_DIR"

if [[ ! -f "$RESULT_FILE" ]]; then
  printf 'timestamp,build_type,seconds\n' > "$RESULT_FILE"
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

run_tests() {
  local build_type="$1"
  local build_dir="$BUILD_ROOT/$build_type"
  local cmake_type
  case "$build_type" in
    debug|Debug)
      cmake_type="Debug"
      ;;
    release|Release)
      cmake_type="Release"
      ;;
    *)
      echo "Unknown build type: $build_type" >&2
      exit 1
      ;;
  esac

  ensure_sdkroot

  cmake -S "$ROOT_DIR" -B "$build_dir" -DCMAKE_BUILD_TYPE="$cmake_type"
  cmake --build "$build_dir"

  local ctest_args=(--progress --output-on-failure)
  ctest_args+=(-C "$cmake_type")

  local start end duration
  start=$(python3 -c 'import time; print(time.time())')
  (cd "$build_dir" && ctest "${ctest_args[@]}")
  end=$(python3 -c 'import time; print(time.time())')
  duration=$(python3 -c "print(f'{$end - $start:.2f}')")

  printf '%s,%s,%s\n' "$(date -Iseconds)" "$cmake_type" "$duration" >> "$RESULT_FILE"
  echo "Recorded $cmake_type tests in ${duration}s"
}

run_tests Debug
run_tests Release
