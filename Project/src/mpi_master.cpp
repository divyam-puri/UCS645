#include "mpi_master.h"
#include "password_generator.h"
#include <mpi.h>
#include <iostream>
#include <iomanip>

MPIMaster::MPIMaster(int numWorkers)
    : numWorkers(numWorkers), currentWorkerId(1) {}

void MPIMaster::distributeWork(
    const std::string& charset,
    int minLen,
    int maxLen,
    const std::string& targetHash,
    const std::string& hashAlgo
) {
    // BUG FIX 1: Use __uint128_t to compute total without overflow, then cap at LLONG_MAX
    __uint128_t total128 = 0;
    __uint128_t cap128   = (__uint128_t)9223372036854775807LL;
    int charsetSize      = charset.length();
    bool overflowed      = false;

    for (int len = minLen; len <= maxLen; ++len) {
        __uint128_t lenCombs = 1;
        for (int i = 0; i < len; ++i) {
            lenCombs *= charsetSize;
            if (lenCombs >= cap128) { overflowed = true; lenCombs = cap128; break; }
        }
        total128 += lenCombs;
        if (total128 >= cap128) { total128 = cap128; overflowed = true; break; }
    }

    long long totalCombinations = (long long)total128;

    if (overflowed) {
        std::cout << "[MASTER] Total combinations: >LLONG_MAX (search space is huge; capped for distribution)" << std::endl;
    } else {
        std::cout << "[MASTER] Total combinations: " << totalCombinations << std::endl;
    }
    std::cout << "[MASTER] Distributing work to " << numWorkers << " workers..." << std::endl;

    // BUG FIX 2: calculateRangeStart always returned 0 — every worker searched index [0, same_end]
    // Fixed: compute correct non-overlapping [start, end] for each worker inline.
    long long workPerWorker = totalCombinations / numWorkers;

    for (int worker = 1; worker <= numWorkers; ++worker) {
        // BUG FIX 3: rangeEnd was totalCombinations/numWorkers for every worker (same value, wrong)
        long long rangeStart = (long long)(worker - 1) * workPerWorker;
        long long rangeEnd   = (worker == numWorkers)
                                 ? totalCombinations - 1
                                 : (long long)worker * workPerWorker - 1;

        MPI_Send((void*)charset.c_str(), charset.length(), MPI_CHAR, worker, 0, MPI_COMM_WORLD);
        MPI_Send(&minLen,      1, MPI_INT,       worker, 1, MPI_COMM_WORLD);
        MPI_Send(&maxLen,      1, MPI_INT,       worker, 2, MPI_COMM_WORLD);
        MPI_Send(&rangeStart,  1, MPI_LONG_LONG, worker, 3, MPI_COMM_WORLD);
        MPI_Send(&rangeEnd,    1, MPI_LONG_LONG, worker, 4, MPI_COMM_WORLD);
        MPI_Send((void*)targetHash.c_str(), targetHash.length(), MPI_CHAR, worker, 5, MPI_COMM_WORLD);
        MPI_Send((void*)hashAlgo.c_str(),   hashAlgo.length(),   MPI_CHAR, worker, 6, MPI_COMM_WORLD);

        std::cout << "[MASTER] Worker " << worker << " assigned range: ["
                  << rangeStart << ", " << rangeEnd << "]" << std::endl;
    }

    std::cout << "[MASTER] Work distribution complete." << std::endl;
}

void MPIMaster::distributeDictionaryWork(
    const std::string& dictFile,
    const std::string& targetHash,
    const std::string& hashAlgo
) {
    std::vector<std::string> dictionary;
    PasswordGenerator::loadDictionary(dictFile, dictionary);

    std::cout << "[MASTER] Loaded dictionary with " << dictionary.size() << " words" << std::endl;

    // Divide dictionary across workers
    size_t wordsPerWorker = dictionary.size() / numWorkers;

    for (int worker = 1; worker <= numWorkers; ++worker) {
        size_t startIdx = (worker - 1) * wordsPerWorker;
        size_t endIdx = (worker == numWorkers) ? dictionary.size() : worker * wordsPerWorker;

        std::cout << "[MASTER] Worker " << worker << " assigned words: ["
                  << startIdx << ", " << endIdx << "]" << std::endl;

        // Send work to worker
        int dictSize = endIdx - startIdx;
        MPI_Send(&dictSize, 1, MPI_INT, worker, 10, MPI_COMM_WORLD);

        for (size_t i = startIdx; i < endIdx; ++i) {
            int wordLen = dictionary[i].length();
            MPI_Send(&wordLen, 1, MPI_INT, worker, 11, MPI_COMM_WORLD);
            MPI_Send((void*)dictionary[i].c_str(), wordLen, MPI_CHAR, worker, 12, MPI_COMM_WORLD);
        }

        MPI_Send((void*)targetHash.c_str(), targetHash.length(), MPI_CHAR, worker, 13, MPI_COMM_WORLD);
        MPI_Send((void*)hashAlgo.c_str(), hashAlgo.length(), MPI_CHAR, worker, 14, MPI_COMM_WORLD);
    }

    std::cout << "[MASTER] Dictionary work distribution complete." << std::endl;
}

std::vector<MPIMaster::WorkerResult> MPIMaster::collectResults() {
    std::vector<WorkerResult> results;

    for (int worker = 1; worker <= numWorkers; ++worker) {
        WorkerResult result;
        result.workerId = worker;

        MPI_Recv(&result.found, 1, MPI_C_BOOL, worker, 20, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&result.hashesComputed, 1, MPI_LONG_LONG, worker, 21, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&result.executionTime, 1, MPI_DOUBLE, worker, 22, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Receive password length first
        int pwdLen = 0;
        MPI_Recv(&pwdLen, 1, MPI_INT, worker, 23, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        if (pwdLen > 0) {
            char pwdBuffer[256];
            MPI_Recv(pwdBuffer, pwdLen, MPI_CHAR, worker, 24, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            result.password = std::string(pwdBuffer, pwdLen);
        }

        results.push_back(result);
        std::cout << "[MASTER] Received result from Worker " << worker << std::endl;
    }

    return results;
}

void MPIMaster::broadcastTermination() {
    std::cout << "[MASTER] Broadcasting termination signal to all workers..." << std::endl;

    for (int worker = 1; worker <= numWorkers; ++worker) {
        bool terminate = true;
        MPI_Send(&terminate, 1, MPI_C_BOOL, worker, 30, MPI_COMM_WORLD);
    }
}

void MPIMaster::waitForPasswordFound(
    std::string& foundPassword,
    int& findingWorker,
    long long& totalHashes
) {
    MPI_Status status;
    bool found = false;

    // Use MPI_Iprobe to check for results
    int flag;
    MPI_Iprobe(MPI_ANY_SOURCE, 40, MPI_COMM_WORLD, &flag, &status);

    if (flag) {
        findingWorker = status.MPI_SOURCE;
        char pwdBuffer[256];
        int pwdLen = 0;

        MPI_Recv(&found, 1, MPI_C_BOOL, findingWorker, 40, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&pwdLen, 1, MPI_INT, findingWorker, 41, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        if (pwdLen > 0) {
            MPI_Recv(pwdBuffer, pwdLen, MPI_CHAR, findingWorker, 42, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            foundPassword = std::string(pwdBuffer, pwdLen);
        }

        MPI_Recv(&totalHashes, 1, MPI_LONG_LONG, findingWorker, 43, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
}

void MPIMaster::printStatistics(
    const std::vector<WorkerResult>& results,
    double totalTime
) {
    std::cout << "\n========== STATISTICS ==========" << std::endl;
    std::cout << "Total execution time: " << std::fixed << std::setprecision(3) << totalTime << " seconds" << std::endl;

    long long totalHashes = 0;
    for (const auto& result : results) {
        totalHashes += result.hashesComputed;
        std::cout << "Worker " << result.workerId << ": " << result.hashesComputed
                  << " hashes (" << (result.hashesComputed / result.executionTime) << " H/s)" << std::endl;
    }

    std::cout << "Total hashes: " << totalHashes << std::endl;
    std::cout << "Overall rate: " << (totalHashes / totalTime) << " hashes/sec" << std::endl;
    std::cout << "==============================\n" << std::endl;
}

long long MPIMaster::calculateRangeStart(int workerId) {
    // NOTE: These helpers are no longer called by distributeWork (fixed inline above).
    // Kept for API compatibility only.
    return 0;
}

long long MPIMaster::calculateRangeEnd(int workerId, long long totalCombinations) {
    // NOTE: These helpers are no longer called by distributeWork (fixed inline above).
    // Kept for API compatibility only.
    return totalCombinations / numWorkers;
}
