#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    srand(time(NULL) + rank);

    int numbers[10];
    int local_max = 0, local_min = 1000;

    for (int i = 0; i < 10; i++) {
        numbers[i] = rand() % 1000;
        if (numbers[i] > local_max) local_max = numbers[i];
        if (numbers[i] < local_min) local_min = numbers[i];
    }

    int global_max, global_min;

    MPI_Reduce(&local_max, &global_max, 1, MPI_INT,
               MPI_MAX, 0, MPI_COMM_WORLD);

    MPI_Reduce(&local_min, &global_min, 1, MPI_INT,
               MPI_MIN, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        printf("Global Maximum = %d\n", global_max);
        printf("Global Minimum = %d\n", global_min);
    }

    MPI_Finalize();
    return 0;
}