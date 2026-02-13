#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include "correlate.h"

int main(int argc, char* argv[])
{
    if(argc < 3)
    {
        std::cout << "Usage: ./corr ny nx\n";
        return 0;
    }

    int ny = atoi(argv[1]);
    int nx = atoi(argv[2]);

    std::vector<float> data(ny * nx);
    std::vector<float> result(ny * ny);

    srand(0);
    for(int i = 0; i < ny*nx; i++)
        data[i] = rand() % 100 / 10.0f;

    clock_t start = clock();

    correlate(ny, nx, data.data(), result.data());

    clock_t end = clock();

    std::cout << "Time: " << (double)(end-start)/CLOCKS_PER_SEC << " seconds\n";

    return 0;
}
