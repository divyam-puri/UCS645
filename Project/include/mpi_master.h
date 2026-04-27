#ifndef MPI_MASTER_H
#define MPI_MASTER_H

#include <string>
#include <vector>
#include <chrono>

class MPIMaster {
public:
    struct WorkerResult {
        int workerId;
        bool found;
        std::string password;
        long long hashesComputed;
        double executionTime;
    };

    MPIMaster(int numWorkers);

    // Distribute work to workers
    void distributeWork(
        const std::string& charset,
        int minLen,
        int maxLen,
        const std::string& targetHash,
        const std::string& hashAlgo
    );

    // Distribute dictionary work
    void distributeDictionaryWork(
        const std::string& dictFile,
        const std::string& targetHash,
        const std::string& hashAlgo
    );

    // Collect results from workers
    std::vector<WorkerResult> collectResults();

    // Broadcast termination signal
    void broadcastTermination();

    // Wait for password found
    void waitForPasswordFound(
        std::string& foundPassword,
        int& findingWorker,
        long long& totalHashes
    );

    // Print statistics
    void printStatistics(
        const std::vector<WorkerResult>& results,
        double totalTime
    );

private:
    int numWorkers;
    int currentWorkerId;

    long long calculateRangeStart(int workerId);
    long long calculateRangeEnd(int workerId, long long totalCombinations);
};

#endif // MPI_MASTER_H
