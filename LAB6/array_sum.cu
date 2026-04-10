#include <stdio.h>

__global__ void sumKernel(float *input, float *result, int n) {
    int idx = threadIdx.x + blockIdx.x * blockDim.x;

    if (idx < n) {
        atomicAdd(result, input[idx]);
    }
}

int main() {
    int n = 1024;
    float *h_arr = (float*)malloc(n * sizeof(float));
    float result = 0;

    for (int i = 0; i < n; i++)
        h_arr[i] = 1.0;

    float *d_arr, *d_result;

    cudaMalloc(&d_arr, n * sizeof(float));
    cudaMalloc(&d_result, sizeof(float));

    cudaMemcpy(d_arr, h_arr, n * sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(d_result, &result, sizeof(float), cudaMemcpyHostToDevice);

    sumKernel<<<(n+255)/256, 256>>>(d_arr, d_result, n);

    cudaMemcpy(&result, d_result, sizeof(float), cudaMemcpyDeviceToHost);

    printf("Sum = %f\n", result);

    cudaFree(d_arr);
    cudaFree(d_result);
    free(h_arr);

    return 0;
}
