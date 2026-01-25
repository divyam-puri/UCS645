#include <iostream>
#include <vector>
#include <omp.h>

#define VECTOR_SIZE 65536   // 2^16

int main() {

    std::vector<double> vectorX(VECTOR_SIZE);
    std::vector<double> vectorY(VECTOR_SIZE);

    double scalarA = 2.5;

    // Step 1: Initialize vectors
    for(int i = 0; i < VECTOR_SIZE; i++) {
        vectorX[i] = i * 1.0;
        vectorY[i] = i * 0.5;
    }

    double startTime = omp_get_wtime();

    // Step 2: Parallel DAXPY operation
    #pragma omp parallel for
    for(int i = 0; i < VECTOR_SIZE; i++) {
        vectorX[i] = scalarA * vectorX[i] + vectorY[i];
    }

    double endTime = omp_get_wtime();

    std::cout << "Execution Time: " << (endTime - startTime) << " seconds\n";

    return 0;
}
