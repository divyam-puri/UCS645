#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define TOTAL_SIZE 500000000   // 500 million elements

int main(int argc, char *argv[]) {

    int rank, size;
    double multiplier;
    long long local_size;
    double local_dot = 0.0;
    double global_dot = 0.0;
    double start, end;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Only rank 0 sets multiplier
    if (rank == 0) {
        multiplier = 2.0;
    }

    // Broadcast multiplier
    MPI_Bcast(&multiplier, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Divide workload
    local_size = TOTAL_SIZE / size;

    MPI_Barrier(MPI_COMM_WORLD);
    start = MPI_Wtime();

    // Local generation + local dot product
    for (long long i = 0; i < local_size; i++) {
        double A = 1.0;
        double B = 2.0 * multiplier;
        local_dot += A * B;
    }

    // Reduce to rank 0
    MPI_Reduce(&local_dot, &global_dot, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);
    end = MPI_Wtime();

    if (rank == 0) {
        printf("Total Dot Product = %.2f\n", global_dot);
        printf("Execution Time = %f seconds\n", end - start);
    }

    MPI_Finalize();
    return 0;
}