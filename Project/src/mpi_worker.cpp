/**
 * mpi_worker.cpp  —  MPI worker process implementation
 * ─────────────────────────────────────────────────────────────────────────────
 * Each worker receives an index range from the master (mpi_master.cpp),
 * then searches that range for a password matching the target hash.
 *
 * Execution path (chosen at runtime):
 *   1. GPU  (CUDA) — enabled when compiled with -DHAVE_CUDA and a GPU is
 *                    present.  Batched kernel launches; ~100–1000× faster.
 *   2. CPU (OpenMP)— fallback when no GPU is available.  Each OpenMP thread
 *                    independently hashes candidates from its portion of the
 *                    assigned range.
 *
 * The MPI communication protocol (receive / send tags) is unchanged from the
 * original; only executeBruteForce() is enhanced.
 */

#include "mpi_worker.h"
#include "password_generator.h"
#include "hash_utils.h"
#include <mpi.h>
#include <omp.h>
#include <iostream>
#include <chrono>
#include <cstring>

#ifdef HAVE_CUDA
#  include "cuda_interface.h"
#endif

/* ─────────────────────────────────────────────────────────────────────────── */

MPIWorker::MPIWorker(int workerId)
    : workerId(workerId)
{
    metrics.hashesComputed  = 0;
    metrics.executionTime   = 0.0;
    metrics.hashesPerSecond = 0.0;
}

/* ── receiveWorkAssignment ─────────────────────────────────────────────────── */
void MPIWorker::receiveWorkAssignment(WorkerState& state)
{
    state.workerId = workerId;

    char charsetBuffer[256] = {};
    MPI_Recv(charsetBuffer, 256, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    state.charset = charsetBuffer;

    MPI_Recv(&state.minLen,   1, MPI_INT,       0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(&state.maxLen,   1, MPI_INT,       0, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(&state.startIdx, 1, MPI_LONG_LONG, 0, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    MPI_Recv(&state.endIdx,   1, MPI_LONG_LONG, 0, 4, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    char hashBuffer[256] = {};
    MPI_Recv(hashBuffer, 256, MPI_CHAR, 0, 5, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    state.targetHash = hashBuffer;

    char algoBuffer[32] = {};
    MPI_Recv(algoBuffer, 32, MPI_CHAR, 0, 6, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    state.hashAlgo = algoBuffer;

    state.running = true;

    std::cout << "[WORKER " << workerId << "] Received assignment: range ["
              << state.startIdx << ", " << state.endIdx << "]" << std::endl;
}

/* ── receiveDictionaryWork ─────────────────────────────────────────────────── */
void MPIWorker::receiveDictionaryWork(std::vector<std::string>& dictionary,
                                      std::string& targetHash,
                                      std::string& hashAlgo)
{
    dictionary.clear();

    int dictSize = 0;
    MPI_Recv(&dictSize, 1, MPI_INT, 0, 10, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    std::cout << "[WORKER " << workerId << "] Receiving " << dictSize << " words" << std::endl;

    for (int i = 0; i < dictSize; ++i) {
        int wordLen = 0;
        MPI_Recv(&wordLen, 1, MPI_INT, 0, 11, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        char wordBuffer[256] = {};
        MPI_Recv(wordBuffer, wordLen, MPI_CHAR, 0, 12, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        dictionary.emplace_back(wordBuffer, wordLen);
    }

    char hashBuffer[256] = {};
    MPI_Recv(hashBuffer, 256, MPI_CHAR, 0, 13, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    targetHash = hashBuffer;

    char algoBuffer[32] = {};
    MPI_Recv(algoBuffer, 32, MPI_CHAR, 0, 14, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    hashAlgo = algoBuffer;

    std::cout << "[WORKER " << workerId << "] Dictionary work received" << std::endl;
}

/* ── executeBruteForce ─────────────────────────────────────────────────────── */
bool MPIWorker::executeBruteForce(const WorkerState& state, std::string& foundPassword)
{
    auto startTime = std::chrono::high_resolution_clock::now();
    long long hashesComputed = 0;
    bool      found          = false;

    HashAlgorithm algo = HashAlgorithm::SHA256;
    if (state.hashAlgo == "MD5")  algo = HashAlgorithm::MD5;
    if (state.hashAlgo == "SHA1") algo = HashAlgorithm::SHA1;

    std::cout << "[WORKER " << workerId << "] Starting brute force "
              << "| range [" << state.startIdx << ", " << state.endIdx << "]"
              << std::endl;

    /* ════════════════════════════════════════════════════════════════════════
     * PATH A — GPU (CUDA)
     * Enabled at compile time with -DHAVE_CUDA and at runtime when a GPU
     * is actually present.  One call to cuda_crack_passwords() launches
     * all necessary batched kernel invocations internally.
     * ════════════════════════════════════════════════════════════════════════ */
#ifdef HAVE_CUDA
    if (cuda_is_available()) {
        std::cout << "[WORKER " << workerId << "] [GPU] CUDA device available — "
                  << "using GPU acceleration" << std::endl;

        /* Print GPU info once per worker for the log */
        {
            char   gpuName[256] = {};
            size_t freeMem = 0, totalMem = 0;
            cuda_get_device_info(0, gpuName, &freeMem, &totalMem);
            std::cout << "[WORKER " << workerId << "] [GPU] "
                      << gpuName
                      << " | Free VRAM: " << freeMem  / (1024 * 1024) << " MB"
                      << " / "            << totalMem / (1024 * 1024) << " MB"
                      << std::endl;
        }

        char gpuFoundPwd[256] = {};
        int  gpuFoundLen      = 0;

        long long hashes = cuda_crack_passwords(
            state.charset.c_str(),
            (int)state.charset.length(),
            state.minLen,
            state.startIdx,
            state.endIdx,
            state.targetHash.c_str(),
            gpuFoundPwd,
            &gpuFoundLen);

        if (hashes >= 0) {
            /* GPU path succeeded */
            hashesComputed = hashes;
            if (gpuFoundLen > 0) {
                found         = true;
                foundPassword = std::string(gpuFoundPwd, gpuFoundLen);
                std::cout << "[WORKER " << workerId << "] [GPU] PASSWORD FOUND: "
                          << foundPassword << std::endl;
            }
            goto record_metrics;   /* skip CPU path */
        }

        /* If hashes < 0, a CUDA error occurred — fall through to CPU */
        std::cout << "[WORKER " << workerId
                  << "] [GPU] CUDA error — falling back to CPU/OpenMP" << std::endl;
    }
#endif   /* HAVE_CUDA */

    /* ════════════════════════════════════════════════════════════════════════
     * PATH B — CPU / OpenMP fallback
     *
     * Each OpenMP thread iterates over a disjoint sub-range of [startIdx,
     * endIdx].  Using  if (found) continue  (rather than  && !found  in the
     * loop condition) is OpenMP-safe; the  reduction  clause correctly
     * accumulates the per-thread hash counts.
     * ════════════════════════════════════════════════════════════════════════ */
    {
        std::cout << "[WORKER " << workerId << "] [CPU] Using OpenMP ("
                  << omp_get_max_threads() << " threads)" << std::endl;

        #pragma omp parallel for schedule(dynamic, 4096) \
            shared(found, foundPassword) reduction(+:hashesComputed)
        for (long long idx = state.startIdx; idx <= state.endIdx; ++idx) {

            if (found) continue;   /* OpenMP-safe early exit */

            std::string password    = PasswordGenerator::indexToPassword(
                                          state.charset, idx, state.minLen);
            std::string computedHash = HashUtils::computeHash(password, algo);
            ++hashesComputed;

            if (computedHash == state.targetHash) {
                #pragma omp critical
                {
                    if (!found) {
                        found         = true;
                        foundPassword = password;
                        std::cout << "[WORKER " << workerId
                                  << "] [CPU] PASSWORD FOUND: " << password
                                  << std::endl;
                    }
                }
            }
        }
    }

#ifdef HAVE_CUDA
record_metrics:
#endif
    {
        auto endTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = endTime - startTime;
        updateMetrics(hashesComputed, elapsed.count());

        std::cout << "[WORKER " << workerId << "] Execution complete: "
                  << hashesComputed << " hashes in "
                  << elapsed.count() << " s ("
                  << (elapsed.count() > 0
                          ? (double)hashesComputed / elapsed.count()
                          : 0.0)
                  << " H/s)" << std::endl;
    }

    return found;
}

/* ── executeDictionaryAttack ───────────────────────────────────────────────── */
bool MPIWorker::executeDictionaryAttack(const std::vector<std::string>& dictionary,
                                        const std::string& targetHash,
                                        const std::string& hashAlgo,
                                        std::string& foundPassword)
{
    auto startTime = std::chrono::high_resolution_clock::now();
    long long hashesComputed = 0;
    bool      found          = false;

    HashAlgorithm algo = HashAlgorithm::SHA256;
    if (hashAlgo == "MD5")  algo = HashAlgorithm::MD5;
    if (hashAlgo == "SHA1") algo = HashAlgorithm::SHA1;

    std::cout << "[WORKER " << workerId << "] Dictionary attack on "
              << dictionary.size() << " words" << std::endl;

    #pragma omp parallel for schedule(dynamic, 512) \
        shared(found, foundPassword) reduction(+:hashesComputed)
    for (size_t i = 0; i < dictionary.size(); ++i) {
        if (found) continue;

        std::string computedHash = HashUtils::computeHash(dictionary[i], algo);
        ++hashesComputed;

        if (computedHash == targetHash) {
            #pragma omp critical
            {
                if (!found) {
                    found         = true;
                    foundPassword = dictionary[i];
                    std::cout << "[WORKER " << workerId
                              << "] PASSWORD FOUND (dict): " << foundPassword
                              << std::endl;
                }
            }
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = endTime - startTime;
    updateMetrics(hashesComputed, elapsed.count());
    return found;
}

/* ── sendResults ───────────────────────────────────────────────────────────── */
void MPIWorker::sendResults(bool found, const std::string& password,
                            long long hashesComputed, double executionTime)
{
    MPI_Send(&found,          1, MPI_C_BOOL,   0, 20, MPI_COMM_WORLD);
    MPI_Send(&hashesComputed, 1, MPI_LONG_LONG,0, 21, MPI_COMM_WORLD);
    MPI_Send(&executionTime,  1, MPI_DOUBLE,   0, 22, MPI_COMM_WORLD);

    int pwdLen = (int)password.length();
    MPI_Send(&pwdLen, 1, MPI_INT, 0, 23, MPI_COMM_WORLD);
    if (pwdLen > 0)
        MPI_Send((void*)password.c_str(), pwdLen, MPI_CHAR, 0, 24, MPI_COMM_WORLD);

    std::cout << "[WORKER " << workerId << "] Results sent to master" << std::endl;
}

/* ── checkTerminationSignal ────────────────────────────────────────────────── */
bool MPIWorker::checkTerminationSignal()
{
    MPI_Status status;
    int flag = 0;
    MPI_Iprobe(0, 30, MPI_COMM_WORLD, &flag, &status);
    if (flag) {
        bool terminate = false;
        MPI_Recv(&terminate, 1, MPI_C_BOOL, 0, 30, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        return terminate;
    }
    return false;
}

/* ── updateMetrics ─────────────────────────────────────────────────────────── */
void MPIWorker::updateMetrics(long long hashes, double time)
{
    metrics.hashesComputed  = hashes;
    metrics.executionTime   = time;
    metrics.hashesPerSecond = (time > 0.0) ? (double)hashes / time : 0.0;
}
