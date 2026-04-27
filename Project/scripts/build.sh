#!/usr/bin/env bash
# build.sh  —  Build the Hybrid MPI + OpenMP + CUDA password cracker
# ─────────────────────────────────────────────────────────────────────────────
# Usage:
#   ./scripts/build.sh              # auto-detect CUDA arch (defaults to sm_75)
#   ./scripts/build.sh 75           # force sm_75 (T4)
#   ./scripts/build.sh 70           # force sm_70 (V100)
#   ./scripts/build.sh 80           # force sm_80 (A100)
# ─────────────────────────────────────────────────────────────────────────────
set -euo pipefail

ARCH="${1:-75}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$ROOT/build"

echo "═══════════════════════════════════════════"
echo " HPC Password Cracker — Build Script"
echo "═══════════════════════════════════════════"

# ── Dependency check ──────────────────────────────────────────────────────────
for dep in cmake make mpicc mpicxx; do
    command -v "$dep" &>/dev/null || { echo "ERROR: $dep not found"; exit 1; }
done

CUDA_AVAILABLE=0
if command -v nvcc &>/dev/null; then
    CUDA_AVAILABLE=1
    echo "[✓] CUDA compiler : $(nvcc --version | head -1)"
else
    echo "[!] nvcc not found — building CPU-only"
fi

echo "[✓] MPI           : $(mpicc --version 2>&1 | head -1)"
echo "[✓] CMake         : $(cmake --version | head -1)"
echo ""

# ── Clean + configure ─────────────────────────────────────────────────────────
rm -rf "$BUILD_DIR"
mkdir  "$BUILD_DIR"
cd     "$BUILD_DIR"

CMAKE_ARGS=("-DCMAKE_BUILD_TYPE=Release")
if [ "$CUDA_AVAILABLE" -eq 1 ]; then
    CMAKE_ARGS+=("-DCMAKE_CUDA_ARCHITECTURES=${ARCH}")
fi

echo "[→] cmake ${CMAKE_ARGS[*]} .."
cmake "${CMAKE_ARGS[@]}" .. 2>&1 | grep -E "(CUDA|MPI|OpenMP|error|warning|HPC)" || true

# ── Compile ───────────────────────────────────────────────────────────────────
NPROC=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
echo ""
echo "[→] make -j${NPROC}"
make -j"${NPROC}" 2>&1

# ── Report ────────────────────────────────────────────────────────────────────
if [ -f "$BUILD_DIR/password_cracker" ]; then
    SIZE=$(du -sh "$BUILD_DIR/password_cracker" | cut -f1)
    echo ""
    echo "═══════════════════════════════════════════"
    echo " ✅  Build successful!  ($SIZE)"
    echo "     Binary: $BUILD_DIR/password_cracker"
    if [ "$CUDA_AVAILABLE" -eq 1 ]; then
        echo "     Mode  : MPI + OpenMP + CUDA (arch sm_${ARCH})"
    else
        echo "     Mode  : MPI + OpenMP (CPU only)"
    fi
    echo "═══════════════════════════════════════════"
else
    echo ""
    echo "❌  Build FAILED — see output above"
    exit 1
fi
