#!/usr/bin/env bash
# ═══════════════════════════════════════════════════════════════════════════════
# crack.sh  —  One-shot: extract → install deps → build → crack
#
# Usage (run from the same directory as the zip):
#   bash crack.sh [OPTIONS]
#
# Options:
#   -p, --password  <str>   Target password          (default: ab3)
#   -c, --charset   <str>   Character set to search  (default: auto by category)
#       --charset-class <class>
#                           Charset shorthand: lower | lower+digits | alphanum | full
#                                             (default: lower+digits)
#   --min-len <n>           Min length               (default: len(password))
#   --max-len <n>           Max length               (default: len(password))
#   --procs   <n>           MPI ranks                (default: 2)
#   --arch    <sm>          CUDA arch, e.g. 75       (default: auto-detect)
#   --skip-install          Skip apt-get (if deps already installed)
#   --skip-build            Skip cmake/make  (reuse existing build/)
#   -h, --help              Show this help
#
# Examples:
#   bash crack.sh                                  # Easy: crack "ab3" instantly
#   bash crack.sh -p mE5@ --charset-class full     # Medium: ~59M candidates
#   bash crack.sh -p Hello7 --charset-class alphanum  # Hard: ~56B candidates
#   bash crack.sh -p secret --charset "abcdefghijklmnopqrstuvwxyz" --min-len 6 --max-len 6
# ═══════════════════════════════════════════════════════════════════════════════
set -euo pipefail

# ── Defaults ──────────────────────────────────────────────────────────────────
PASSWORD="ab3"
CHARSET_CLASS="lower+digits"
CHARSET_OVERRIDE=""
MIN_LEN=""
MAX_LEN=""
NUM_PROCS=2
CUDA_ARCH=""
SKIP_INSTALL=0
SKIP_BUILD=0

# ── Charset definitions ───────────────────────────────────────────────────────
CS_LOWER="abcdefghijklmnopqrstuvwxyz"
CS_UPPER="ABCDEFGHIJKLMNOPQRSTUVWXYZ"
CS_DIGITS="0123456789"
CS_SPECIAL="!@#\$%^&*()-_=+[]{}|;:,.<>?"

# ── Parse args ────────────────────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
    case "$1" in
        -p|--password)      PASSWORD="$2";         shift 2 ;;
        -c|--charset)       CHARSET_OVERRIDE="$2"; shift 2 ;;
        --charset-class)    CHARSET_CLASS="$2";    shift 2 ;;
        --min-len)          MIN_LEN="$2";          shift 2 ;;
        --max-len)          MAX_LEN="$2";          shift 2 ;;
        --procs)            NUM_PROCS="$2";        shift 2 ;;
        --arch)             CUDA_ARCH="$2";        shift 2 ;;
        --skip-install)     SKIP_INSTALL=1;        shift ;;
        --skip-build)       SKIP_BUILD=1;          shift ;;
        -h|--help)
            sed -n '2,30p' "$0" | sed 's/^# \{0,1\}//'
            exit 0 ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

# ── Resolve charset ───────────────────────────────────────────────────────────
if [[ -n "$CHARSET_OVERRIDE" ]]; then
    CHARSET="$CHARSET_OVERRIDE"
else
    case "$CHARSET_CLASS" in
        lower)          CHARSET="$CS_LOWER" ;;
        lower+digits)   CHARSET="${CS_LOWER}${CS_DIGITS}" ;;
        upper+digits)   CHARSET="${CS_UPPER}${CS_DIGITS}" ;;
        alphanum)       CHARSET="${CS_LOWER}${CS_UPPER}${CS_DIGITS}" ;;
        full)           CHARSET="${CS_LOWER}${CS_UPPER}${CS_DIGITS}${CS_SPECIAL}" ;;
        *)
            echo "Unknown --charset-class '$CHARSET_CLASS'."
            echo "Choose: lower | lower+digits | upper+digits | alphanum | full"
            exit 1 ;;
    esac
fi

PWD_LEN=${#PASSWORD}
[[ -z "$MIN_LEN" ]] && MIN_LEN=$PWD_LEN
[[ -z "$MAX_LEN" ]] && MAX_LEN=$PWD_LEN

# ── Locate project root (where this script lives) ─────────────────────────────
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

banner() { echo ""; echo "══════════════════════════════════════════════════════"; echo "  $*"; echo "══════════════════════════════════════════════════════"; }

banner "🔐 Hybrid MPI + OpenMP + CUDA Password Cracker"
echo "  Password  : $PASSWORD"
echo "  Charset   : $CHARSET  (${#CHARSET} chars)"
echo "  Length    : ${MIN_LEN}–${MAX_LEN}"
SPACE=$(python3 -c "print(f'{${#CHARSET}**${MAX_LEN}:,}')" 2>/dev/null || echo "?")
echo "  Space     : ${#CHARSET}^${MAX_LEN} = $SPACE candidates"
echo "  MPI ranks : $NUM_PROCS"

# ═════════════════════════════════════════════════════════════════════════════
# STEP 1 — Install dependencies
# ═════════════════════════════════════════════════════════════════════════════
if [[ $SKIP_INSTALL -eq 0 ]]; then
    banner "📦 Step 1 — Installing dependencies"
    if command -v apt-get &>/dev/null; then
        apt-get update -qq
        apt-get install -y -qq cmake libopenmpi-dev openmpi-bin libomp-dev build-essential
        echo "  ✅ apt packages installed"
    else
        echo "  ⚠️  Not an apt system — skipping package install."
        echo "  Make sure cmake, openmpi, and libomp are installed."
    fi
else
    echo ""; echo "  [skip-install] Skipping apt-get."
fi

# ═════════════════════════════════════════════════════════════════════════════
# STEP 2 — Ensure nvcc is on PATH (Bug 1 fix)
# ═════════════════════════════════════════════════════════════════════════════
banner "🔍 Step 2 — Detecting CUDA"

for cuda_bin in /usr/local/cuda/bin /usr/local/cuda-12/bin /usr/local/cuda-11/bin /usr/local/cuda-10/bin; do
    if [[ -d "$cuda_bin" && ":$PATH:" != *":$cuda_bin:"* ]]; then
        export PATH="$cuda_bin:$PATH"
        echo "  🔧 Added $cuda_bin to PATH"
    fi
done

CUDA_AVAILABLE=0
NVCC_PATH=""
if command -v nvcc &>/dev/null; then
    CUDA_AVAILABLE=1
    NVCC_PATH=$(command -v nvcc)
    echo "  ✅ nvcc found: $NVCC_PATH"
    nvcc --version | grep "release"
else
    echo "  ⚠️  nvcc not found — will build CPU-only (MPI + OpenMP)"
fi

# Auto-detect CUDA arch via nvidia-smi
if [[ -z "$CUDA_ARCH" ]]; then
    if command -v nvidia-smi &>/dev/null; then
        CAP=$(nvidia-smi --query-gpu=compute_cap --format=csv,noheader 2>/dev/null \
              | head -1 | tr -d '.' | xargs)
        [[ -n "$CAP" ]] && CUDA_ARCH="$CAP" || CUDA_ARCH="75"
    else
        CUDA_ARCH="75"
    fi
    echo "  ✅ CUDA arch: sm_${CUDA_ARCH}"
fi

if command -v nvidia-smi &>/dev/null; then
    nvidia-smi --query-gpu=name,memory.total,compute_cap --format=csv,noheader 2>/dev/null \
        | sed 's/^/  GPU: /' || true
fi

# ═════════════════════════════════════════════════════════════════════════════
# STEP 3 — Build
# ═════════════════════════════════════════════════════════════════════════════
if [[ $SKIP_BUILD -eq 0 ]]; then
    banner "🔨 Step 3 — Building (cmake + make)"

    rm -rf build && mkdir build
    cd build

    CMAKE_CMD="cmake .. -DCMAKE_BUILD_TYPE=Release"
    if [[ $CUDA_AVAILABLE -eq 1 ]]; then
        CMAKE_CMD="$CMAKE_CMD -DCMAKE_CUDA_ARCHITECTURES=${CUDA_ARCH}"
        CMAKE_CMD="$CMAKE_CMD -DCMAKE_CUDA_COMPILER=${NVCC_PATH}"
    fi

    echo "  [1/2] $CMAKE_CMD"
    eval "$CMAKE_CMD" 2>&1 | grep -E "(HPC|CUDA|MPI|OpenMP|[Ee]rror)" || true

    NPROC=$(nproc 2>/dev/null || echo 4)
    echo "  [2/2] make -j${NPROC}"
    make -j"${NPROC}"
    cd ..

    if [[ ! -f "build/password_cracker" ]]; then
        echo ""; echo "❌ Build failed — see output above"; exit 1
    fi

    SZ=$(du -sh build/password_cracker | cut -f1)
    CUDA_SYMS=$(nm build/password_cracker 2>/dev/null | grep -c cuda || echo 0)
    echo ""
    echo "  ✅ Build successful! ($SZ)"
    if [[ "$CUDA_SYMS" -gt 0 ]]; then
        echo "  ✅ CUDA symbols linked ($CUDA_SYMS) — GPU path active"
    else
        echo "  ⚠️  No CUDA symbols — CPU/OpenMP fallback will be used"
    fi
else
    echo ""; echo "  [skip-build] Reusing existing build/"
    [[ ! -f "build/password_cracker" ]] && { echo "❌ No binary at build/password_cracker"; exit 1; }
fi

# ═════════════════════════════════════════════════════════════════════════════
# STEP 4 — Run
# ═════════════════════════════════════════════════════════════════════════════
banner "🚀 Step 4 — Cracking"

CMD="mpirun --allow-run-as-root --oversubscribe -np ${NUM_PROCS} \
  ./build/password_cracker \
  --password \"${PASSWORD}\" \
  --min-len ${MIN_LEN} \
  --max-len ${MAX_LEN} \
  --charset \"${CHARSET}\""

echo "  CMD: $CMD"
echo ""

eval "$CMD"

echo ""
banner "✅ Done"
