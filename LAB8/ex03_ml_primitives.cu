#include <stdio.h>
#include <cuda.h>
#include <math.h>

#define N 1024

// ================= ACTIVATIONS =================

// Sigmoid
__global__ void sigmoid(float *x, float *y, int n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        y[i] = 1.0f / (1.0f + expf(-x[i]));
    }
}

// Tanh
__global__ void tanh_kernel(float *x, float *y, int n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        y[i] = tanhf(x[i]);
    }
}

// ReLU
__global__ void relu(float *x, float *y, int n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        y[i] = (x[i] > 0) ? x[i] : 0;
    }
}

// ReLU backward
__global__ void relu_backward(float *x, float *grad, float *out, int n) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        out[i] = (x[i] > 0) ? grad[i] : 0;
    }
}

// ================= LOSS =================

// Cross entropy
__global__ void cross_entropy(float *pred, float *target, float *loss, int n) {
    int i = threadIdx.x;
    if (i < n) {
        loss[i] = -target[i] * logf(pred[i] + 1e-7);
    }
}

// ================= MAIN =================

int main() {

    float h_x[N], h_sig[N], h_tanh[N], h_relu[N];
    float h_grad[N], h_relu_back[N];
    float h_pred[N], h_target[N], h_loss[N];

    // Initialize
    for (int i = 0; i < N; i++) {
        h_x[i] = (float)i / N;   // 0 → 1 range
        h_grad[i] = 1.0f;
        h_pred[i] = 0.9f;
        h_target[i] = 1.0f;
    }

    float *d_x, *d_sig, *d_tanh, *d_relu;
    float *d_grad, *d_relu_back;
    float *d_pred, *d_target, *d_loss;

    cudaMalloc(&d_x, N*sizeof(float));
    cudaMalloc(&d_sig, N*sizeof(float));
    cudaMalloc(&d_tanh, N*sizeof(float));
    cudaMalloc(&d_relu, N*sizeof(float));
    cudaMalloc(&d_grad, N*sizeof(float));
    cudaMalloc(&d_relu_back, N*sizeof(float));
    cudaMalloc(&d_pred, N*sizeof(float));
    cudaMalloc(&d_target, N*sizeof(float));
    cudaMalloc(&d_loss, N*sizeof(float));

    cudaMemcpy(d_x, h_x, N*sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(d_grad, h_grad, N*sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(d_pred, h_pred, N*sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(d_target, h_target, N*sizeof(float), cudaMemcpyHostToDevice);

    int threads = 256;
    int blocks = (N + threads - 1)/threads;

    // ================= RUN EACH SEPARATELY =================

    // Sigmoid
    sigmoid<<<blocks, threads>>>(d_x, d_sig, N);
    cudaMemcpy(h_sig, d_sig, N*sizeof(float), cudaMemcpyDeviceToHost);

    // Tanh
    tanh_kernel<<<blocks, threads>>>(d_x, d_tanh, N);
    cudaMemcpy(h_tanh, d_tanh, N*sizeof(float), cudaMemcpyDeviceToHost);

    // ReLU
    relu<<<blocks, threads>>>(d_x, d_relu, N);
    cudaMemcpy(h_relu, d_relu, N*sizeof(float), cudaMemcpyDeviceToHost);

    // ReLU backward
    relu_backward<<<blocks, threads>>>(d_x, d_grad, d_relu_back, N);
    cudaMemcpy(h_relu_back, d_relu_back, N*sizeof(float), cudaMemcpyDeviceToHost);

    // Cross entropy
    cross_entropy<<<1, N>>>(d_pred, d_target, d_loss, N);
    cudaMemcpy(h_loss, d_loss, N*sizeof(float), cudaMemcpyDeviceToHost);

    // ================= PRINT =================

    printf("Sigmoid sample: %f\n", h_sig[0]);
    printf("Tanh sample: %f\n", h_tanh[0]);
    printf("ReLU sample: %f\n", h_relu[0]);
    printf("ReLU backward sample: %f\n", h_relu_back[0]);
    printf("Loss sample: %f\n", h_loss[0]);

    // ================= CLEANUP =================

    cudaFree(d_x);
    cudaFree(d_sig);
    cudaFree(d_tanh);
    cudaFree(d_relu);
    cudaFree(d_grad);
    cudaFree(d_relu_back);
    cudaFree(d_pred);
    cudaFree(d_target);
    cudaFree(d_loss);

    return 0;
}