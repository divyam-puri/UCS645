#include <stdio.h>
#include <cuda_runtime.h>

int main() {
    int deviceCount;
    cudaGetDeviceCount(&deviceCount);

    printf("Number of CUDA Devices: %d\n\n", deviceCount);

    for (int i = 0; i < deviceCount; i++) {
        cudaDeviceProp prop;
        cudaGetDeviceProperties(&prop, i);

        printf("Device %d: %s\n", i, prop.name);
        printf("Compute Capability: %d.%d\n", prop.major, prop.minor);
        printf("Max Threads per Block: %d\n", prop.maxThreadsPerBlock);
        printf("Max Block Dimensions: %d x %d x %d\n",
               prop.maxThreadsDim[0], prop.maxThreadsDim[1], prop.maxThreadsDim[2]);
        printf("Max Grid Dimensions: %d x %d x %d\n",
               prop.maxGridSize[0], prop.maxGridSize[1], prop.maxGridSize[2]);
        printf("Global Memory: %ld MB\n", prop.totalGlobalMem / (1024*1024));
        printf("Shared Memory per Block: %ld KB\n", prop.sharedMemPerBlock / 1024);
        printf("Constant Memory: %ld KB\n", prop.totalConstMem / 1024);
        printf("Warp Size: %d\n", prop.warpSize);
        printf("Double Precision Support: %s\n\n",
               (prop.major >= 1) ? "Yes" : "No");
    }
    return 0;
}
