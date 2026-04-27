#include <stdio.h>
#include <cuda.h>

#define N 8
#define K 3

// ================= CONVOLUTION =================
__global__ void conv2d(float *input, float *kernel, float *output) {
    int i = threadIdx.x;
    int j = threadIdx.y;

    float sum = 0.0;
    for(int ki=0; ki<K; ki++) {
        for(int kj=0; kj<K; kj++) {
            sum += input[(i+ki)*N + (j+kj)] * kernel[ki*K + kj];
        }
    }

    output[i*N + j] = sum;
}

// ================= RELU =================
__global__ void relu(float *x, int size) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if(i < size) {
        if(x[i] < 0) x[i] = 0;
    }
}

// ================= MAXPOOL =================
__global__ void maxpool(float *input, float *output) {
    int i = threadIdx.x;
    int j = threadIdx.y;

    int row = i * 2;
    int col = j * 2;

    float max_val = input[row*N + col];

    for(int di=0; di<2; di++) {
        for(int dj=0; dj<2; dj++) {
            float val = input[(row+di)*N + (col+dj)];
            if(val > max_val) max_val = val;
        }
    }

    output[i*(N/2) + j] = max_val;
}

// ================= MAIN =================
int main() {

    float h_input[N*N], h_kernel[K*K], h_output[N*N];

    for(int i=0;i<N*N;i++) h_input[i] = 1;
    for(int i=0;i<K*K;i++) h_kernel[i] = 1;

    float *d_input, *d_kernel, *d_output;
    cudaMalloc(&d_input, N*N*sizeof(float));
    cudaMalloc(&d_kernel, K*K*sizeof(float));
    cudaMalloc(&d_output, N*N*sizeof(float));

    cudaMemcpy(d_input, h_input, N*N*sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(d_kernel, h_kernel, K*K*sizeof(float), cudaMemcpyHostToDevice);

    dim3 threads(N-K+1, N-K+1);

    conv2d<<<1, threads>>>(d_input, d_kernel, d_output);

    relu<<<(N*N+255)/256, 256>>>(d_output, N*N);

    float *d_pool;
    cudaMalloc(&d_pool, (N/2)*(N/2)*sizeof(float));

    dim3 pool_threads(N/2, N/2);
    maxpool<<<1, pool_threads>>>(d_output, d_pool);

    cudaMemcpy(h_output, d_pool, (N/2)*(N/2)*sizeof(float), cudaMemcpyDeviceToHost);

    printf("CNN output sample: %f\n", h_output[0]);

    cudaFree(d_input);
    cudaFree(d_kernel);
    cudaFree(d_output);
    cudaFree(d_pool);

    return 0;
}