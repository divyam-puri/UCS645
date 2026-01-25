#include <iostream>
#include <omp.h>

#define SIZE 1000

int main() {

    static double matrixA[SIZE][SIZE];
    static double matrixB[SIZE][SIZE];
    static double matrixC[SIZE][SIZE];

    for(int i = 0; i < SIZE; i++) {
        for(int j = 0; j < SIZE; j++) {
            matrixA[i][j] = 1.0;
            matrixB[i][j] = 1.0;
            matrixC[i][j] = 0.0;
        }
    }

    double startTime = omp_get_wtime();

    #pragma omp parallel for
    for(int i = 0; i < SIZE; i++) {
        for(int j = 0; j < SIZE; j++) {
            for(int k = 0; k < SIZE; k++) {
                matrixC[i][j] += matrixA[i][k] * matrixB[k][j];
            }
        }
    }

    double endTime = omp_get_wtime();

    std::cout << "1D Parallel Time: " << endTime - startTime << " seconds\n";
    return 0;
}
