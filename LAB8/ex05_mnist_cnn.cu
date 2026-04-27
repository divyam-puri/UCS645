#include <stdio.h>
#include <cuda.h>

#define N 16
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

// ================= FULLY CONNECTED =================
__global__ void fully_connected(float *input, float *weights, float *output, int n) {
    int i = threadIdx.x;
    float sum = 0;

    for(int j=0; j<n; j++) {
        sum += input[j] * weights[i*n + j];
    }

    output[i] = sum;
}

// ================= MAIN =================
int main() {

    float h_input[N*N], h_kernel[K*K], h_conv[N*N];
    float h_fc_in[N*N], h_weights[N*N], h_fc_out[N];

    for(int i=0;i<N*N;i++) {
        h_input[i] = 1.0;
        h_fc_in[i] = 1.0;
    }

    for(int i=0;i<K*K;i++) h_kernel[i] = 1.0;
    for(int i=0;i<N*N;i++) h_weights[i] = 0.5;

    float *d_input, *d_kernel, *d_conv;
    float *d_fc_in, *d_weights, *d_fc_out;

    cudaMalloc(&d_input, N*N*sizeof(float));
    cudaMalloc(&d_kernel, K*K*sizeof(float));
    cudaMalloc(&d_conv, N*N*sizeof(float));
    cudaMalloc(&d_fc_in, N*N*sizeof(float));
    cudaMalloc(&d_weights, N*N*sizeof(float));
    cudaMalloc(&d_fc_out, N*sizeof(float));

    cudaMemcpy(d_input, h_input, N*N*sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(d_kernel, h_kernel, K*K*sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(d_fc_in, h_fc_in, N*N*sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(d_weights, h_weights, N*N*sizeof(float), cudaMemcpyHostToDevice);

    // Convolution
    dim3 threads(N-K+1, N-K+1);
    conv2d<<<1, threads>>>(d_input, d_kernel, d_conv);

    // ReLU
    relu<<<(N*N+255)/256,256>>>(d_conv, N*N);

    // Fully connected
    fully_connected<<<1, N>>>(d_fc_in, d_weights, d_fc_out, N);

    cudaMemcpy(h_fc_out, d_fc_out, N*sizeof(float), cudaMemcpyDeviceToHost);

    printf("CNN final output sample: %f\n", h_fc_out[0]);

    cudaFree(d_input);
    cudaFree(d_kernel);
    cudaFree(d_conv);
    cudaFree(d_fc_in);
    cudaFree(d_weights);
    cudaFree(d_fc_out);

    return 0;
}