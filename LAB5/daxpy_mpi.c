#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define N 65536   // 2^16

int main(int argc, char *argv[]) {

    int rank, size;
    double *X = NULL, *Y = NULL;
    double *local_X, *local_Y;
    double a = 2.5;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int local_n = N / size;

    local_X = (double*) malloc(local_n * sizeof(double));
    local_Y = (double*) malloc(local_n * sizeof(double));

    if (rank == 0) {
        X = (double*) malloc(N * sizeof(double));
        Y = (double*) malloc(N * sizeof(double));

        for (int i = 0; i < N; i++) {
            X[i] = 1.0;
            Y[i] = 2.0;
        }
    }

    MPI_Scatter(X, local_n, MPI_DOUBLE, local_X, local_n,
                MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Scatter(Y, local_n, MPI_DOUBLE, local_Y, local_n,
                MPI_DOUBLE, 0, MPI_COMM_WORLD);

    double start = MPI_Wtime();

    for (int i = 0; i < local_n; i++) {
        local_X[i] = a * local_X[i] + local_Y[i];
    }

    double end = MPI_Wtime();

    if (rank == 0) {
        printf("Execution Time (MPI): %f seconds\n", end - start);
    }

    MPI_Finalize();
    return 0;
}