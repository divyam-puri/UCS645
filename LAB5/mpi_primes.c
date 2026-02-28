#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define MAX 50000
#define TERMINATE -1

int is_prime(int n) {
    if (n < 2) return 0;
    for (int i = 2; i <= sqrt(n); i++) {
        if (n % i == 0)
            return 0;
    }
    return 1;
}

int main(int argc, char *argv[]) {

    int rank, size;
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {

        int next_number = 2;
        int active_slaves = size - 1;
        int received_value;

        double start = MPI_Wtime();

        while (active_slaves > 0) {

            MPI_Recv(&received_value, 1, MPI_INT,
                     MPI_ANY_SOURCE, 0,
                     MPI_COMM_WORLD, &status);

            int slave = status.MPI_SOURCE;

            if (received_value > 0) {
                printf("Prime found: %d\n", received_value);
            }

            if (next_number <= MAX) {
                MPI_Send(&next_number, 1, MPI_INT,
                         slave, 0, MPI_COMM_WORLD);
                next_number++;
            } else {
                int terminate = TERMINATE;
                MPI_Send(&terminate, 1, MPI_INT,
                         slave, 0, MPI_COMM_WORLD);
                active_slaves--;
            }
        }

        double end = MPI_Wtime();
        printf("Execution Time = %f seconds\n", end - start);

    } else {

        int number;
        int result;

        int request = 0;

        while (1) {

            MPI_Send(&request, 1, MPI_INT,
                     0, 0, MPI_COMM_WORLD);

            MPI_Recv(&number, 1, MPI_INT,
                     0, 0, MPI_COMM_WORLD, &status);

            if (number == TERMINATE)
                break;

            if (is_prime(number))
                result = number;
            else
                result = -number;

            request = result;
        }
    }

    MPI_Finalize();
    return 0;
}