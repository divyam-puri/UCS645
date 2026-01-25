# UCS645 – OpenMP Lab 1

## Aim
To understand the basics of OpenMP and shared memory parallel programming by implementing
parallel algorithms using multiple threads.

## Prerequisites
- C++ Compiler with OpenMP support
- Linux / macOS (with LLVM + libomp)
- Basic knowledge of C++ and Linux commands

## Compilation Command

All programs are compiled using:

clang++ filename.cpp -fopenmp -O2 -o output

Example:
clang++ daxpy_openmp.cpp -fopenmp -O2 -o daxpy

## Execution Command

To run with different number of threads:

OMP_NUM_THREADS=2 ./daxpy  
OMP_NUM_THREADS=4 ./daxpy  
OMP_NUM_THREADS=8 ./daxpy  

Same format is used for all programs.

---

## Programs Included

### 1. DAXPY Operation (daxpy_openmp.cpp)

Operation performed:
X[i] = a * X[i] + Y[i]

Vector size = 2^16

Purpose:
To study speedup obtained by increasing number of threads.

Observation:
Speedup increases till number of CPU cores. After that, performance does not improve
due to memory bandwidth limitation and thread management overhead.

---

### 2. Matrix Multiplication – 1D Parallel (matrix_mul_1d.cpp)

Matrix Size: 1000 x 1000

Parallelization:
Only outer loop (row-wise) is parallelized.

Each thread computes different rows of result matrix.

Observation:
Some threads finish earlier leading to less efficient load balancing.

---

### 3. Matrix Multiplication – 2D Parallel (matrix_mul_2d.cpp)

Parallelization:
Two loops are parallelized using collapse(2) directive.

Threads share both rows and columns.

Observation:
Better workload distribution and improved performance compared to 1D parallelization.

---

### 4. Calculation of Pi using Numerical Integration (pi_openmp.cpp)

Formula used:

pi = integral from 0 to 1 of (4 / (1 + x^2)) dx

Method:
Area under curve using rectangles.

Parallelization:
Loop is parallelized using reduction clause.

Purpose of Reduction:
To safely combine partial sums from all threads and avoid race condition.

---

## Conclusion

OpenMP allows easy parallelization of loops using simple directives.
Performance improves with increase in threads up to hardware limits.
Proper workload distribution and use of reduction is important for correct
and efficient parallel programs.

---

## Folder Structure

UCS645  
 └── LAB1  
     ├── daxpy_openmp.cpp  
     ├── matrix_mul_1d.cpp  
     ├── matrix_mul_2d.cpp  
     ├── pi_openmp.cpp  
     └── README.md  

---

## Submitted By

Name: Divyam Puri
Roll Number: 102483023
Group: 3C43
Course: UCS645 (Parallel & Distributed Computing)  
Lab: OpenMP Lab 1

