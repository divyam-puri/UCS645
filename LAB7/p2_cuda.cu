#include <stdio.h>

__global__ void simpleSort(int *arr) {
    int i = threadIdx.x;

    for (int j = i + 1; j < 1000; j++) {
        if (arr[i] > arr[j]) {
            int temp = arr[i];
            arr[i] = arr[j];
            arr[j] = temp;
        }
    }
}

int main() {
    int h_arr[1000];
    int *d_arr;

    for (int i = 0; i < 1000; i++)
        h_arr[i] = 1000 - i;

    cudaMalloc(&d_arr, sizeof(int)*1000);
    cudaMemcpy(d_arr, h_arr, sizeof(int)*1000, cudaMemcpyHostToDevice);

    simpleSort<<<1,1000>>>(d_arr);

    cudaMemcpy(h_arr, d_arr, sizeof(int)*1000, cudaMemcpyDeviceToHost);

    printf("Sorted first element: %d\n", h_arr[0]);

    cudaFree(d_arr);
}
