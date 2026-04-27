/**
 * cuda_hash.cu  —  GPU-accelerated password cracking kernels
 * ─────────────────────────────────────────────────────────────────────────────
 * Three-layer parallelism:
 *   MPI   → one process (worker) per rank, distributes index ranges
 *   OpenMP → CPU threads inside each worker (fallback when no GPU)
 *   CUDA  → thousands of GPU threads per worker (primary path)
 *
 * Design:
 *  • Passwords are generated entirely on the GPU from a global index —
 *    no host-side candidate list is needed, eliminating the CPU→GPU transfer.
 *  • Hash algorithm mirrors HashUtils::computeSHA256() bit-for-bit so that
 *    the target hash computed on CPU is directly comparable on the GPU.
 *  • Work processed in BATCH_SIZE (2^20 ≈ 1 M) candidates per kernel launch.
 *  • A single atomicCAS-guarded int (d_foundFlag) serialises the result write.
 */

#include <cuda_runtime.h>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include "cuda_interface.h"

/* ───── Error-checking macros ─────────────────────────────────────────────── */
#define CUDA_CHECK(call)                                                        \
    do {                                                                        \
        cudaError_t _e = (call);                                                \
        if (_e != cudaSuccess) {                                                \
            fprintf(stderr, "[CUDA] %s:%d — %s\n",                             \
                    __FILE__, __LINE__, cudaGetErrorString(_e));                \
            return -1LL;                                                        \
        }                                                                       \
    } while (0)

/* ═══════════════════════════════════════════════════════════════════════════
 * Device helper: global index → password string
 *
 * Mirrors PasswordGenerator::indexToPassword() exactly.
 * Index space is ordered by increasing length:
 *   [0 .. cs^minLen)          → all passwords of length minLen
 *   [cs^minLen .. cs^minLen + cs^(minLen+1)) → length minLen+1, …
 * ═══════════════════════════════════════════════════════════════════════════ */
__device__ __forceinline__
void d_indexToPassword(const char* __restrict__ charset,
                       int      charsetLen,
                       uint64_t idx,
                       int      minLen,
                       char*    password,
                       int*     pwLen)
{
    /* Find which length bucket contains idx */
    uint64_t cumulative = 0;
    int      pwl        = minLen;

    for (;;) {
        uint64_t count    = 1;
        bool     overflow = false;
        for (int i = 0; i < pwl; ++i) {
            if (count > UINT64_MAX / (uint64_t)charsetLen) { overflow = true; break; }
            count *= (uint64_t)charsetLen;
        }
        if (overflow || cumulative + count > idx) break;
        cumulative += count;
        ++pwl;
        if (pwl > 32) { *pwLen = 0; return; }   /* safety cap */
    }

    *pwLen = pwl;

    /* Extract base-charsetLen digits, fill right-to-left */
    uint64_t offset = idx - cumulative;
    for (int i = pwl - 1; i >= 0; --i) {
        password[i] = charset[offset % (uint64_t)charsetLen];
        offset      /= (uint64_t)charsetLen;
    }
    password[pwl] = '\0';
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Device helper: password → 64-char hex hash
 *
 * Identical to HashUtils::computeSHA256() on Linux x86-64:
 *   unsigned long hash = 0;
 *   for each char c: hash = c + (hash<<6) + (hash<<16) - hash;
 *   format as 64-char lowercase hex, left-padded with '0'
 *
 * (unsigned long == uint64_t on Linux 64-bit; same arithmetic on GPU)
 * ═══════════════════════════════════════════════════════════════════════════ */
__device__ __forceinline__
void d_computeHash(const char* __restrict__ password,
                   int                      pwLen,
                   char* __restrict__       hexOut)
{
    uint64_t hash = 0;
    for (int i = 0; i < pwLen; ++i) {
        /* Signed char → int (CPU behaviour) → uint64_t — matches host exactly */
        uint64_t c = (uint64_t)(int)password[i];
        hash = c + (hash << 6) + (hash << 16) - hash;
    }

    const char hx[] = "0123456789abcdef";

    /* First 48 chars: padding zeros */
    for (int i = 0; i < 48; ++i) hexOut[i] = '0';

    /* Last 16 chars: the 64-bit hash value */
    uint64_t h = hash;
    for (int i = 63; i >= 48; --i) {
        hexOut[i] = hx[h & 0xFULL];
        h >>= 4;
    }
    hexOut[64] = '\0';
}

/* ── Compare two 64-char hex strings ─────────────────────────────────────── */
__device__ __forceinline__
bool d_hashMatch(const char* __restrict__ a, const char* __restrict__ b)
{
    for (int i = 0; i < 64; ++i)
        if (a[i] != b[i]) return false;
    return true;
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Main CUDA kernel
 *
 * Grid/block: 1-D layout.  Thread tid handles global index (batchStart+tid).
 * ═══════════════════════════════════════════════════════════════════════════ */
__global__
void crackPasswordsKernel(const char* __restrict__ d_charset,
                          int      charsetLen,
                          int      minLen,
                          uint64_t batchStart,
                          uint64_t batchSize,
                          const char* __restrict__ d_targetHash,
                          int*  d_foundFlag,
                          char* d_foundPassword,
                          int*  d_foundLen)
{
    uint64_t tid = (uint64_t)blockIdx.x * blockDim.x + threadIdx.x;
    if (tid >= batchSize)  return;
    if (*d_foundFlag)      return;   /* early exit: another thread found it */

    char password[33] = {};
    int  pwLen        = 0;
    d_indexToPassword(d_charset, charsetLen, batchStart + tid, minLen,
                      password, &pwLen);
    if (pwLen == 0) return;

    char hashHex[65] = {};
    d_computeHash(password, pwLen, hashHex);

    if (d_hashMatch(hashHex, d_targetHash)) {
        if (atomicCAS(d_foundFlag, 0, 1) == 0) {   /* first finder wins */
            for (int i = 0; i <= pwLen; ++i)
                d_foundPassword[i] = password[i];
            *d_foundLen = pwLen;
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Host-side C interface  (called from mpi_worker.cpp)
 * ═══════════════════════════════════════════════════════════════════════════ */
extern "C" {

bool cuda_is_available()
{
    int count = 0;
    return (cudaGetDeviceCount(&count) == cudaSuccess && count > 0);
}

void cuda_get_device_info(int device_id, char* name,
                          size_t* free_mem, size_t* total_mem)
{
    cudaDeviceProp prop;
    if (cudaGetDeviceProperties(&prop, device_id) != cudaSuccess) {
        snprintf(name, 256, "Unknown");
        if (free_mem)  *free_mem  = 0;
        if (total_mem) *total_mem = 0;
        return;
    }
    strncpy(name, prop.name, 255);  name[255] = '\0';
    cudaSetDevice(device_id);
    size_t f = 0, t = 0;
    cudaMemGetInfo(&f, &t);
    if (free_mem)  *free_mem  = f;
    if (total_mem) *total_mem = t;
}

void cuda_print_device_info()
{
    int count = 0;
    if (cudaGetDeviceCount(&count) != cudaSuccess || count == 0) {
        printf("[CUDA] No GPU devices found\n");
        return;
    }
    for (int i = 0; i < count; ++i) {
        cudaDeviceProp prop;
        cudaGetDeviceProperties(&prop, i);
        cudaSetDevice(i);
        size_t fmem = 0, tmem = 0;
        cudaMemGetInfo(&fmem, &tmem);
        printf("[CUDA] Device %d: %-32s | Compute %d.%d | SMs:%3d | VRAM: %.1f/%.1f GB\n",
               i, prop.name, prop.major, prop.minor,
               prop.multiProcessorCount,
               (double)fmem / 1073741824.0,
               (double)tmem / 1073741824.0);
        fflush(stdout);
    }
}

long long cuda_crack_passwords(const char* charset,
                               int         charsetLen,
                               int         minLen,
                               long long   startIdx,
                               long long   endIdx,
                               const char* targetHash,
                               char*       foundPassword,
                               int*        foundLen)
{
    if (!cuda_is_available()) return -1LL;

    const int      BLOCK_SIZE = 256;
    const uint64_t BATCH_SIZE = 1ULL << 23;   /* ✅ FIX: 8M/launch → fewer cudaDeviceSynchronize calls */   /* 1 048 576 candidates/launch */

    uint64_t uStart = (uint64_t)startIdx;
    uint64_t uEnd   = (uint64_t)endIdx;
    if (uEnd < uStart) { *foundLen = 0; return 0LL; }

    /* ── Persistent device allocations ────────────────────────────────────── */
    char* d_charset       = nullptr;
    char* d_targetHash    = nullptr;
    int*  d_foundFlag     = nullptr;
    char* d_foundPassword = nullptr;
    int*  d_foundLen      = nullptr;

    CUDA_CHECK(cudaMalloc(&d_charset,       (size_t)(charsetLen + 1)));
    CUDA_CHECK(cudaMalloc(&d_targetHash,    65));
    CUDA_CHECK(cudaMalloc(&d_foundFlag,     sizeof(int)));
    CUDA_CHECK(cudaMalloc(&d_foundPassword, 256));  /* ✅ FIX: match interface docs (was 64) */
    CUDA_CHECK(cudaMalloc(&d_foundLen,      sizeof(int)));

    CUDA_CHECK(cudaMemcpy(d_charset,   charset,    (size_t)(charsetLen + 1), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_targetHash,targetHash, 65,                        cudaMemcpyHostToDevice));

    int zero = 0;
    CUDA_CHECK(cudaMemcpy(d_foundFlag, &zero, sizeof(int), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_foundLen,  &zero, sizeof(int), cudaMemcpyHostToDevice));

    /* ── Batch loop ───────────────────────────────────────────────────────── */
    long long hashesComputed = 0;

    for (uint64_t bStart = uStart; bStart <= uEnd; bStart += BATCH_SIZE) {

        /* Poll: did a previous batch find it? */
        int h_flag = 0;
        CUDA_CHECK(cudaMemcpy(&h_flag, d_foundFlag, sizeof(int), cudaMemcpyDeviceToHost));
        if (h_flag) break;

        uint64_t bSize = BATCH_SIZE;
        if (bStart + bSize - 1 > uEnd) bSize = uEnd - bStart + 1;

        int grid = (int)((bSize + BLOCK_SIZE - 1) / BLOCK_SIZE);

        crackPasswordsKernel<<<grid, BLOCK_SIZE>>>(
            d_charset, charsetLen, minLen,
            bStart, bSize,
            d_targetHash,
            d_foundFlag, d_foundPassword, d_foundLen);

        CUDA_CHECK(cudaDeviceSynchronize());
        hashesComputed += (long long)bSize;
    }

    /* ── Copy result back ────────────────────────────────────────────────── */
    int h_flag = 0, h_len = 0;
    CUDA_CHECK(cudaMemcpy(&h_flag, d_foundFlag, sizeof(int), cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemcpy(&h_len,  d_foundLen,  sizeof(int), cudaMemcpyDeviceToHost));

    if (h_flag && h_len > 0) {
        CUDA_CHECK(cudaMemcpy(foundPassword, d_foundPassword,
                              (size_t)(h_len + 1), cudaMemcpyDeviceToHost));
        foundPassword[h_len] = '\0';
        *foundLen = h_len;
    } else {
        *foundLen = 0;
    }

    /* ── Cleanup ─────────────────────────────────────────────────────────── */
    cudaFree(d_charset);
    cudaFree(d_targetHash);
    cudaFree(d_foundFlag);
    cudaFree(d_foundPassword);
    cudaFree(d_foundLen);

    return hashesComputed;
}

} /* extern "C" */
