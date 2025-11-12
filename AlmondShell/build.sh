#!/bin/bash
# Usage: ./build.sh [gcc|clang] [Debug|Release] [-- cmake args]

set -euo pipefail

if [[ $# -lt 1 ]]; then
  echo "Usage: $0 [gcc|clang] [Debug|Release] [-- cmake args]" >&2
  exit 1
fi

COMPILER_CHOICE=$1
shift

if [[ $# -gt 0 && $1 != "--" ]]; then
  BUILD_TYPE=$1
  shift
else
  BUILD_TYPE=Debug
fi

if [[ $# -gt 0 && $1 == "--" ]]; then
  shift
  EXTRA_CMAKE_ARGS=("$@")
else
  EXTRA_CMAKE_ARGS=()
fi

case "$COMPILER_CHOICE" in
  gcc)
    COMPILER_C="gcc"
    COMPILER_CXX="g++"
    COMPILER_NAME="GCC"
    ;;
  clang)
    COMPILER_C="clang"
    COMPILER_CXX="clang++"
    COMPILER_NAME="Clang"
    ;;
  *)
    echo "Unsupported compiler '$COMPILER_CHOICE'. Use 'gcc' or 'clang'." >&2
    exit 1
    ;;
esac

INSTALL_PREFIX="${PWD}/built"
BUILD_DIR="Bin/${COMPILER_NAME}-${BUILD_TYPE}"

cmake_args=(
  -S .
  -B "$BUILD_DIR"
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
  -DCMAKE_C_COMPILER="$COMPILER_C"
  -DCMAKE_CXX_COMPILER="$COMPILER_CXX"
  -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX"
)

cmake_args+=("${EXTRA_CMAKE_ARGS[@]}")

cmake "${cmake_args[@]}"

cmake --build "$BUILD_DIR" --verbose

if cmake -LA -N "$BUILD_DIR" | grep -q "DOXYGEN_FOUND:BOOL=1"; then
  echo "Generating AlmondShell API documentation..."
  if cmake --build "$BUILD_DIR" --target docs; then
    echo "API reference available under $(pwd)/docs/api/html/index.html"
  else
    echo "Doxygen reported an error while generating documentation." >&2
  fi
else
  echo "Skipping API documentation generation (Doxygen not detected during configure)."
fi
