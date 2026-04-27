#ifndef MPI_WORKER_H
#define MPI_WORKER_H

#include <string>
#include <vector>

class MPIWorker {
public:
    struct WorkerState {
        int workerId;
        long long startIdx;
        long long endIdx;
        std::string charset;
        int minLen;
        int maxLen;
        std::string targetHash;
        std::string hashAlgo;
        bool running;
    };

    MPIWorker(int workerId);

    // Receive work assignment from master
    void receiveWorkAssignment(WorkerState& state);

    // Receive dictionary work
    void receiveDictionaryWork(
        std::vector<std::string>& dictionary,
        std::string& targetHash,
        std::string& hashAlgo
    );

    // Execute brute force password cracking
    bool executeBruteForce(const WorkerState& state, std::string& foundPassword);

    // Execute dictionary attack
    bool executeDictionaryAttack(
        const std::vector<std::string>& dictionary,
        const std::string& targetHash,
        const std::string& hashAlgo,
        std::string& foundPassword
    );

    // Send results back to master
    void sendResults(
        bool found,
        const std::string& password,
        long long hashesComputed,
        double executionTime
    );

    // Check termination signal
    bool checkTerminationSignal();

    // Performance metrics
    struct PerformanceMetrics {
        long long hashesComputed;
        double executionTime;
        double hashesPerSecond;
    };

    PerformanceMetrics getMetrics() const { return metrics; }

private:
    int workerId;
    PerformanceMetrics metrics;

    void updateMetrics(long long hashes, double time);
};

#endif // MPI_WORKER_H
