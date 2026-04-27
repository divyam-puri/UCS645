#!/bin/bash

# Benchmark script for password cracker performance evaluation

set -e

echo "=========================================="
echo "Password Cracker Performance Benchmark"
echo "=========================================="
echo ""

if [ ! -f "build/password_cracker" ]; then
    echo "ERROR: Executable not found. Run ./scripts/build.sh first"
    exit 1
fi

# Test 1: CPU only (1 process, single thread)
echo "Test 1: CPU Only (1 MPI process, 1 thread)"
echo "=========================================="
time OMP_NUM_THREADS=1 mpirun -np 1 ./build/password_cracker
echo ""

# Test 2: CPU + OpenMP (1 process, 4 threads)
echo "Test 2: CPU + OpenMP (1 MPI process, 4 threads)"
echo "=========================================="
time OMP_NUM_THREADS=4 mpirun -np 1 ./build/password_cracker
echo ""

# Test 3: CPU + OpenMP (1 process, 8 threads)
echo "Test 3: CPU + OpenMP (1 MPI process, 8 threads)"
echo "=========================================="
time OMP_NUM_THREADS=8 mpirun -np 1 ./build/password_cracker
echo ""

# Test 4: MPI + OpenMP (4 processes, 4 threads each)
echo "Test 4: MPI + OpenMP (4 MPI processes, 4 threads each)"
echo "=========================================="
time OMP_NUM_THREADS=4 mpirun -np 4 ./build/password_cracker
echo ""

# Test 5: MPI + OpenMP (8 processes, 4 threads each)
echo "Test 5: MPI + OpenMP (8 MPI processes, 4 threads each)"
echo "=========================================="
time OMP_NUM_THREADS=4 mpirun -np 8 ./build/password_cracker
echo ""

echo "=========================================="
echo "Benchmark complete!"
echo "=========================================="
echo ""
echo "Interpretation:"
echo "- CPU Only: Single-threaded baseline"
echo "- CPU+OpenMP: Multi-threaded speedup"
echo "- MPI: Distributed speedup"
echo "- MPI+OpenMP: Hybrid speedup"
echo ""
echo "Expected speedup factors:"
echo "- OpenMP (8T): ~6-8x on 8-core CPU"
echo "- MPI (4 nodes): ~3.5-4x"
echo "- MPI+OpenMP (4x8T): ~24-32x"
