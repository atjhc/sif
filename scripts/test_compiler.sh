#!/usr/bin/env bash
set -euo pipefail

# Usage: scripts/test_compiler.sh [clang|gcc] [debug|release]
# Runs unit tests with the specified compiler and build configuration.
#
# Examples:
#   scripts/test_compiler.sh clang debug
#   scripts/test_compiler.sh gcc release
#   scripts/test_compiler.sh gcc debug
#
# If no arguments provided, shows usage and lists available options.

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

show_usage() {
  echo "Usage: $0 [compiler] [build-type]"
  echo ""
  echo "Compilers:"
  echo "  clang    - Build and test with Clang (native)"
  echo "  gcc      - Build and test with GCC 13 (Docker)"
  echo ""
  echo "Build types:"
  echo "  debug    - Debug build with symbols"
  echo "  release  - Release build with optimizations"
  echo ""
  echo "Examples:"
  echo "  $0 clang debug"
  echo "  $0 gcc release"
  echo "  $0 gcc debug"
  exit 1
}

run_clang_tests() {
  local build_type="$1"
  local make_target

  case "$build_type" in
    debug|Debug)
      make_target="test"  # defaults to debug
      ;;
    release|Release)
      make_target="test_release"
      ;;
    *)
      echo "Unknown build type: $build_type" >&2
      exit 1
      ;;
  esac

  echo "================================================"
  echo "Building with Clang ($build_type)..."
  echo "================================================"

  cd "$ROOT_DIR"
  rm -rf build
  make "$make_target"
}

run_gcc_tests() {
  local build_type="$1"
  local make_target
  local build_dir

  case "$build_type" in
    debug|Debug)
      make_target="debug"
      build_dir="debug"
      ;;
    release|Release)
      make_target="release"
      build_dir="release"
      ;;
    *)
      echo "Unknown build type: $build_type" >&2
      exit 1
      ;;
  esac

  echo "================================================"
  echo "Building with GCC 13 ($build_type) in Docker..."
  echo "================================================"

  # Check if Docker is available
  if ! command -v docker >/dev/null 2>&1; then
    echo "Error: docker command not found" >&2
    echo "Docker is required to run GCC tests" >&2
    exit 1
  fi

  # Build the GCC Docker image if needed
  if ! docker images gcc-image:latest | grep -q gcc-image; then
    echo "Building GCC Docker image..."
    docker build -f "$ROOT_DIR/Dockerfile.gcc" -t gcc-image:latest "$ROOT_DIR"
  fi

  # Run build and tests in Docker
  docker run --rm \
    -v "$ROOT_DIR:/mnt/build" \
    -w /mnt/build \
    gcc-image:latest \
    bash -c "rm -rf build && make $make_target && cd build/$build_dir && ctest --output-on-failure"
}

# Parse arguments
if [[ $# -ne 2 ]]; then
  show_usage
fi

COMPILER="${1:-}"
BUILD_TYPE="${2:-}"

# Normalize compiler name
case "$COMPILER" in
  clang|Clang|CLANG)
    run_clang_tests "$BUILD_TYPE"
    ;;
  gcc|GCC|Gcc)
    run_gcc_tests "$BUILD_TYPE"
    ;;
  *)
    echo "Error: Unknown compiler '$COMPILER'" >&2
    echo ""
    show_usage
    ;;
esac
