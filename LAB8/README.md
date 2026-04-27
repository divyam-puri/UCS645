# 🚀 Assignment 8 (LAB8) – GPU Accelerated Machine Learning

## 👤 Student Details
- **Name:** Divyam Puri  
- **Course:** UCS645 – Parallel Computing  
- **Platform:** NVIDIA CUDA (Tesla T4 – Google Colab)

---

## 📂 Repository Structure

    LAB8/
    ├── ex01_cuda_basics.cu
    ├── ex02_memory_hierarchy.cu
    ├── ex03_ml_primitives.cu
    ├── ex04_cnn_layers.cu
    |── ex05_mnist_cnn.cu
    |── Assignment8_Report.docx
    └── README.md

---

## 🔧 How to Run

    nvcc -O2 -arch=sm_75 ex01_cuda_basics.cu -o ex01
    ./ex01

---

## 🧪 Exercises Covered

### 🔹 ex01 – CUDA Basics
- CPU vs GPU comparison
- Speedup analysis

### 🔹 ex02 – Memory Hierarchy
- Shared memory optimization
- Reduction techniques

### 🔹 ex03 – ML Primitives
- Sigmoid, Tanh, ReLU, Loss

### 🔹 ex04 – CNN Layers
- Convolution + Pooling

### 🔹 ex05 – CNN Pipeline
- Simplified forward pass

---

## 📈 Results

- GPU faster for large inputs
- CPU better for small inputs
- Parallelism improves performance

---

## 🏁 Conclusion

CUDA enables efficient parallel computing for machine learning workloads.
