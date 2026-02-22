#include <mpi.h>
#include <stdio.h>

#define N 8

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int A[N] = {1,2,3,4,5,6,7,8};
    int B[N] = {8,7,6,5,4,3,2,1};

    int chunk = N / size;
    int local_A[chunk], local_B[chunk];
    int local_sum = 0, global_sum = 0;

    MPI_Scatter(A, chunk, MPI_INT,
                local_A, chunk, MPI_INT,
                0, MPI_COMM_WORLD);

    MPI_Scatter(B, chunk, MPI_INT,
                local_B, chunk, MPI_INT,
                0, MPI_COMM_WORLD);

    for (int i = 0; i < chunk; i++)
        local_sum += local_A[i] * local_B[i];

    MPI_Reduce(&local_sum, &global_sum, 1, MPI_INT,
               MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0)
        printf("Dot Product = %d\n", global_sum);

    MPI_Finalize();
    return 0;
}