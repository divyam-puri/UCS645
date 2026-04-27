#include <stdio.h>
#include <cuda.h>

__global__ void reduce_shared(float *input, float *output) {
    __shared__ float sdata[256];

    int tid = threadIdx.x;
    int i = blockIdx.x * blockDim.x + tid;

    sdata[tid] = input[i];
    __syncthreads();

    for(int s = blockDim.x/2; s > 0; s >>= 1) {
        if(tid < s) {
            sdata[tid] += sdata[tid + s];
        }
        __syncthreads();
    }

    if(tid == 0)
        output[blockIdx.x] = sdata[0];
}

int main() {
    int N = 1<<20;
    size_t size = N * sizeof(float);

    float *h = (float*)malloc(size);
    for(int i=0;i<N;i++) h[i] = 1.0;

    float *d_in, *d_out;
    cudaMalloc(&d_in, size);
    cudaMalloc(&d_out, size/256);

    cudaMemcpy(d_in, h, size, cudaMemcpyHostToDevice);

    reduce_shared<<<N/256,256>>>(d_in, d_out);

    printf("Reduction done\n");

    cudaFree(d_in);
    cudaFree(d_out);
}