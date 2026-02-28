#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define ARRAY_SIZE 10000000   // 10 million doubles (~80MB)

void MyBcast(double *buffer, int count, int rank, int size) {

    if (rank == 0) {
        for (int i = 1; i < size; i++) {
            MPI_Send(buffer, count, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);
        }
    } else {
        MPI_Recv(buffer, count, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
}

int main(int argc, char *argv[]) {

    int rank, size;
    double *array;
    double start, end;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    array = (double*) malloc(ARRAY_SIZE * sizeof(double));

    // Initialize only on root
    if (rank == 0) {
        for (long i = 0; i < ARRAY_SIZE; i++)
            array[i] = 1.0;
    }

    // -----------------------------
    // Part A – Manual Broadcast
    // -----------------------------
    MPI_Barrier(MPI_COMM_WORLD);
    start = MPI_Wtime();

    MyBcast(array, ARRAY_SIZE, rank, size);

    MPI_Barrier(MPI_COMM_WORLD);
    end = MPI_Wtime();

    if (rank == 0)
        printf("MyBcast Time: %f seconds\n", end - start);

    // -----------------------------
    // Part B – MPI_Bcast
    // -----------------------------
    MPI_Barrier(MPI_COMM_WORLD);
    start = MPI_Wtime();

    MPI_Bcast(array, ARRAY_SIZE, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);
    end = MPI_Wtime();

    if (rank == 0)
        printf("MPI_Bcast Time: %f seconds\n", end - start);

    free(array);
    MPI_Finalize();
    return 0;
}