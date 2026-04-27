#include <stdio.h>
#include <cuda.h>

__global__ void vectorAdd(float *A, float *B, float *C, int N) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < N) {
        C[i] = A[i] + B[i];
    }
}

void cpu_add(float *A, float *B, float *C, int N) {
    for(int i=0;i<N;i++){
        C[i] = A[i] + B[i];
    }
}

int main() {
    int sizes[] = {1024, 16384, 262144, 4194304, 67108864};

    for(int s=0;s<5;s++){
        int N = sizes[s];
        size_t size = N * sizeof(float);

        float *A = (float*)malloc(size);
        float *B = (float*)malloc(size);
        float *C = (float*)malloc(size);
        float *C_cpu = (float*)malloc(size);

        for(int i=0;i<N;i++){
            A[i] = 1.0;
            B[i] = 2.0;
        }

        // CPU
        clock_t start = clock();
        cpu_add(A,B,C_cpu,N);
        clock_t end = clock();
        float cpu_time = ((float)(end-start))/CLOCKS_PER_SEC;

        // GPU
        float *d_A, *d_B, *d_C;
        cudaMalloc(&d_A, size);
        cudaMalloc(&d_B, size);
        cudaMalloc(&d_C, size);

        cudaMemcpy(d_A, A, size, cudaMemcpyHostToDevice);
        cudaMemcpy(d_B, B, size, cudaMemcpyHostToDevice);

        cudaEvent_t start_gpu, stop_gpu;
        cudaEventCreate(&start_gpu);
        cudaEventCreate(&stop_gpu);

        cudaEventRecord(start_gpu);

        int threads = 256;
        int blocks = (N + threads - 1)/threads;

        vectorAdd<<<blocks, threads>>>(d_A, d_B, d_C, N);

        cudaEventRecord(stop_gpu);
        cudaEventSynchronize(stop_gpu);

        float gpu_time;
        cudaEventElapsedTime(&gpu_time, start_gpu, stop_gpu);

        // Bandwidth
        float bytes = N * sizeof(float) * 2;
        float bandwidth = bytes / (gpu_time * 1e6);

        float speedup = cpu_time / (gpu_time / 1000.0);

        printf("N=%d | CPU=%f sec | GPU=%f ms | BW=%f GB/s | Speedup=%f\n",
               N, cpu_time, gpu_time, bandwidth, speedup);

        cudaFree(d_A); cudaFree(d_B); cudaFree(d_C);
        free(A); free(B); free(C); free(C_cpu);
    }
}