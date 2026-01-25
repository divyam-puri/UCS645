#include <iostream>
#include <omp.h>

static long numberOfSteps = 100000000;

int main() {

    double stepSize = 1.0 / (double) numberOfSteps;
    double totalSum = 0.0;

    double startTime = omp_get_wtime();

    // Parallel integration with reduction
    #pragma omp parallel for reduction(+:totalSum)
    for(long i = 0; i < numberOfSteps; i++) {
        double x = (i + 0.5) * stepSize;
        totalSum += 4.0 / (1.0 + x * x);
    }

    double piValue = stepSize * totalSum;

    double endTime = omp_get_wtime();

    std::cout << "Calculated Pi = " << piValue << "\n";
    std::cout << "Execution Time = " << (endTime - startTime) << " seconds\n";

    return 0;
}
