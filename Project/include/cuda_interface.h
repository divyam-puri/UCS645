/**
 * cuda_interface.h
 * ─────────────────────────────────────────────────────────────────────────────
 * Clean C-linkage boundary between the C++ MPI/OpenMP layer and the CUDA
 * layer.  All symbols here are callable from plain C++ without needing any
 * CUDA headers in the caller's translation unit.
 *
 * Compile-time guard: this header is only meaningful when HAVE_CUDA is
 * defined (set by CMake when nvcc is found).  mpi_worker.cpp wraps every
 * call inside  #ifdef HAVE_CUDA  so the project compiles cleanly without GPU.
 * ─────────────────────────────────────────────────────────────────────────────
 */
#pragma once
#include <cstddef>   // size_t

#ifdef __cplusplus
extern "C" {
#endif

/* ── Device detection ─────────────────────────────────────────────────────── */

/**
 * Returns true (1) if at least one CUDA-capable GPU is present and
 * accessible; false (0) otherwise.
 */
bool cuda_is_available();

/**
 * Fills  name[256]  with the device name and writes free / total VRAM (bytes)
 * for the GPU with the given  device_id  (usually 0 on Colab).
 */
void cuda_get_device_info(int     device_id,
                          char*   name,       /* out: at least 256 bytes */
                          size_t* free_mem,   /* out: bytes free          */
                          size_t* total_mem); /* out: bytes total         */

/**
 * Prints a one-line summary for every detected GPU to stdout.
 * Called by the master process at start-up.
 */
void cuda_print_device_info();


/* ── Core GPU cracking function ───────────────────────────────────────────── */

/**
 * GPU-accelerated brute-force password search.
 *
 * Searches the global index range  [startIdx, endIdx]  for a password whose
 * hash (computed with the same algorithm as HashUtils::computeSHA256) matches
 * the 64-character hex string  targetHash.
 *
 * Passwords are generated entirely on the GPU using the same
 * indexToPassword mapping as the CPU path — no host-side generation needed.
 * Work is processed in batches of 2^20 (~1 M) candidates per kernel launch
 * to keep the GPU saturated while remaining responsive.
 *
 * @param charset        Null-terminated character set (host memory).
 * @param charsetLen     strlen(charset)  — passed explicitly to avoid a
 *                       device-side strlen.
 * @param minLen         Minimum password length (mirrors the CPU setting).
 * @param startIdx       First global index to search  (inclusive).
 * @param endIdx         Last  global index to search  (inclusive).
 * @param targetHash     64-char lowercase hex string to match (host memory).
 *                       Must be null-terminated (65 bytes total).
 * @param foundPassword  Output buffer — must be at least 256 bytes.
 *                       Written only when the password is found.
 * @param foundLen       Output: length of found password, or 0 if not found.
 *
 * @return  Total number of hashes computed (>= 0), or -1 on CUDA error.
 */
long long cuda_crack_passwords(
    const char* charset,
    int         charsetLen,
    int         minLen,
    long long   startIdx,
    long long   endIdx,
    const char* targetHash,
    char*       foundPassword,   /* out: >= 256 bytes */
    int*        foundLen         /* out */
);

#ifdef __cplusplus
}
#endif
