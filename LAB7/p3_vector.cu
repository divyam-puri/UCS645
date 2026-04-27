#include <stdio.h>
#include <cuda.h>

#define N 1024

__global__ void vectorAdd(float *A, float *B, float *C) {
    int i = threadIdx.x + blockIdx.x * blockDim.x;
    if (i < N)
        C[i] = A[i] + B[i];
}

int main() {
    float A[N], B[N], C[N];
    float *d_A, *d_B, *d_C;

    for (int i = 0; i < N; i++) {
        A[i] = 1.0;
        B[i] = 2.0;
    }

    cudaMalloc(&d_A, sizeof(float)*N);
    cudaMalloc(&d_B, sizeof(float)*N);
    cudaMalloc(&d_C, sizeof(float)*N);

    cudaMemcpy(d_A, A, sizeof(float)*N, cudaMemcpyHostToDevice);
    cudaMemcpy(d_B, B, sizeof(float)*N, cudaMemcpyHostToDevice);

    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);

    cudaEventRecord(start);

    vectorAdd<<<1, N>>>(d_A, d_B, d_C);

    cudaEventRecord(stop);
    cudaEventSynchronize(stop);

    float time;
    cudaEventElapsedTime(&time, start, stop);

    cudaMemcpy(C, d_C, sizeof(float)*N, cudaMemcpyDeviceToHost);

    printf("Execution Time: %f ms\n", time);
    printf("C[0] = %f\n", C[0]);

    // Bandwidth calculation
    float bytes = 3 * N * sizeof(float);
    float bandwidth = bytes / (time/1000.0);

    printf("Measured Bandwidth: %f GB/s\n", bandwidth / 1e9);

    cudaFree(d_A); cudaFree(d_B); cudaFree(d_C);
}
