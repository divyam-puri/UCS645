#include <mpi.h>
#include <omp.h>
#include <iostream>
#include <string>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include "mpi_master.h"
#include "mpi_worker.h"
#include "password_generator.h"
#include "hash_utils.h"

// ─────────────────────────────────────────────────────────────────────────────
// main.cpp  —  Entry point for the hybrid MPI + OpenMP + CUDA password cracker
//
// CLI arguments (all optional, with sensible defaults):
//   --password <str>    Target password. Hash is computed here at runtime.
//   --min-len  <int>    Minimum candidate length  (default: length of password)
//   --max-len  <int>    Maximum candidate length  (default: length of password)
//   --charset  <str>    Character set to search   (default: CHARSET_FULL)
//
// No values are baked in at compile time — parameters come from the command
// line, so the binary never needs to be recompiled between runs.
// ─────────────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // ── Parse CLI arguments ───────────────────────────────────────────────────
    std::string targetPassword = "test";   // sensible demo default
    std::string charset        = PasswordGenerator::CHARSET_FULL;
    int         minLen         = -1;       // -1 = "derive from password length"
    int         maxLen         = -1;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "--password" || arg == "-p") && i + 1 < argc) {
            targetPassword = argv[++i];
        } else if ((arg == "--min-len" || arg == "--min") && i + 1 < argc) {
            minLen = std::atoi(argv[++i]);
        } else if ((arg == "--max-len" || arg == "--max") && i + 1 < argc) {
            maxLen = std::atoi(argv[++i]);
        } else if ((arg == "--charset" || arg == "-c") && i + 1 < argc) {
            charset = argv[++i];
        }
    }

    // Derive lengths from password if not explicitly set
    if (minLen < 1) minLen = (int)targetPassword.length();
    if (maxLen < 1) maxLen = (int)targetPassword.length();

    // ── Compute target hash from the password (on every rank — cheap) ─────────
    // Uses the same polynomial as hash_utils.cpp / cuda_hash.cu
    std::string targetHash = HashUtils::computeHash(targetPassword, HashAlgorithm::SHA256);

    auto totalStartTime = std::chrono::high_resolution_clock::now();

    if (rank == 0) {
        // ── MASTER PROCESS ────────────────────────────────────────────────────
        std::cout << "\n============================================" << std::endl;
        std::cout << "Hybrid Parallel Distributed Password Cracker" << std::endl;
        std::cout << "Team: VibeCoders | Course: UCS645"            << std::endl;
        std::cout << "============================================\n" << std::endl;

        std::cout << "[MASTER] MPI initialized with " << size << " processes" << std::endl;
        std::cout << "[MASTER] OpenMP threads per process: " << omp_get_max_threads() << std::endl;

        std::cout << "[MASTER] Target password : " << targetPassword    << std::endl;
        std::cout << "[MASTER] Target hash     : " << targetHash        << std::endl;
        std::cout << "[MASTER] Charset (" << charset.length() << " chars): " << charset << std::endl;
        std::cout << "[MASTER] Search space    : len " << minLen << "--" << maxLen << std::endl;

        MPIMaster master(size - 1);

        std::cout << "\n[MASTER] === PHASE 1: WORK DISTRIBUTION ===" << std::endl;
        master.distributeWork(charset, minLen, maxLen, targetHash, "SHA256");

        std::cout << "\n[MASTER] === PHASE 2: RESULT COLLECTION ===" << std::endl;
        auto results = master.collectResults();

        std::cout << "\n[MASTER] === PHASE 3: RESULT PROCESSING ===" << std::endl;
        bool        passwordFound = false;
        std::string foundPassword;
        int         findingWorker = -1;

        for (const auto& result : results) {
            if (result.found) {
                passwordFound = true;
                foundPassword = result.password;
                findingWorker = result.workerId;
                break;
            }
        }

        auto totalEndTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> totalElapsed = totalEndTime - totalStartTime;

        std::cout << "\n========== FINAL RESULTS ==========" << std::endl;
        if (passwordFound) {
            std::cout << "PASSWORD FOUND: " << foundPassword << std::endl;
            std::cout << "Found by Worker: " << findingWorker << std::endl;
        } else {
            std::cout << "PASSWORD NOT FOUND in search space" << std::endl;
        }

        master.printStatistics(results, totalElapsed.count());

    } else {
        // ── WORKER PROCESS ────────────────────────────────────────────────────
        MPIWorker worker(rank);

        std::cout << "[WORKER " << rank << "] Worker initialized" << std::endl;

        MPIWorker::WorkerState state;
        worker.receiveWorkAssignment(state);

        std::string foundPassword;
        bool found = worker.executeBruteForce(state, foundPassword);

        auto metrics = worker.getMetrics();

        worker.sendResults(
            found,
            foundPassword,
            metrics.hashesComputed,
            metrics.executionTime
        );

        std::cout << "[WORKER " << rank << "] Execution complete. "
                  << metrics.hashesComputed << " hashes computed in "
                  << metrics.executionTime << " seconds ("
                  << metrics.hashesPerSecond << " H/s)" << std::endl;
    }

    MPI_Finalize();
    return 0;
}
