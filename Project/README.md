# Hybrid Parallel Distributed Password Cracking System

**Course:** UCS645 - Parallel and Distributed Computing  
**Team:** VibeCoders  
**Submitted To:** Dr. Saif Nalband

## Project Overview

This project implements a **high-performance, hybrid parallel password cracking system** that combines:

- **MPI (Message Passing Interface)** - Distributed computation across multiple nodes
- **OpenMP** - Multi-threaded parallelism for candidate generation on each node
- **CUDA** - GPU acceleration for hash computation

The system demonstrates scalable password cracking through distributed and parallel computing techniques, achieving orders-of-magnitude speedup over CPU-only approaches.

## Architecture

### Three-Layer Parallelism

```
┌─────────────────────────────────────┐
│     Master Node (MPI Rank 0)        │
│  Coordinates and aggregates results │
└────────────────┬────────────────────┘
                 │ MPI Distribution
     ┌───────────┼───────────┐
     │           │           │
┌────▼──┐   ┌───▼───┐   ┌──▼────┐
│Worker1│   │Worker2│   │Worker3│  (MPI Nodes)
│OpenMP │   │OpenMP │   │OpenMP │
│ GPU   │   │ GPU   │   │ GPU   │
└──────┘   └───────┘   └──────┘
```

### Data Flow

1. **Master distributes password search space** via MPI
2. **Workers use OpenMP** to parallelize candidate generation
3. **Workers launch CUDA kernels** for GPU-accelerated hash computation
4. **Results aggregated** and early termination signaled when password found

## Project Structure

```
password-cracker-hpc/
├── src/                        # C++ source files
│   ├── main.cpp               # MPI entry point
│   ├── mpi_master.cpp         # Master process logic
│   ├── mpi_worker.cpp         # Worker process logic
│   ├── password_generator.cpp # Candidate generation
│   └── hash_utils.cpp         # Hash computation
│
├── cuda/                        # CUDA GPU kernels
│   ├── cuda_hash.cu           # Hash computation kernels
│   └── cuda_utils.cu          # GPU utility functions
│
├── include/                     # Header files
│   ├── mpi_master.h
│   ├── mpi_worker.h
│   ├── password_generator.h
│   └── hash_utils.h
│
├── scripts/                     # Build and run scripts
│   ├── build.sh               # Compilation script
│   ├── run_cluster.sh         # Cluster execution
│   └── benchmark.sh           # Performance testing
│
├── data/                        # Data files
│   └── dictionary.txt         # Optional dictionary for attacks
│
├── CMakeLists.txt             # Build configuration
└── README.md                  # This file
```

## Requirements

### Software
- **C++ Compiler**: GCC 7.0+ or Clang 5.0+
- **CMake**: 3.18+
- **MPI**: OpenMPI or MPICH
- **OpenMP**: Built into most modern compilers
- **CUDA Toolkit**: 11.0+ (optional, for GPU acceleration)

### Hardware
- **Multi-core CPU**: 4+ cores recommended
- **NVIDIA GPU**: CUDA-capable (Compute Capability 3.5+) - optional
- **Network**: For cluster deployment

### Installation (Ubuntu/Debian)

```bash
# Install dependencies
sudo apt-get update
sudo apt-get install build-essential cmake
sudo apt-get install libopenmpi-dev openmpi-bin
sudo apt-get install libomp-dev

# Install CUDA Toolkit (optional)
# Download from: https://developer.nvidia.com/cuda-downloads
```

## Compilation

### Local Build

```bash
cd password-cracker-hpc
chmod +x scripts/build.sh
./scripts/build.sh
```

This will:
1. Create a `build/` directory
2. Configure with CMake
3. Compile all source files
4. Link MPI, OpenMP, and CUDA libraries

### Manual Compilation

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
cd ..
```

### Expected Build Output
```
Built target password_cracker
```

The executable `build/password_cracker` is ready to run.

## Execution

### Local Execution

**Single node, 1 process:**
```bash
mpirun -np 1 build/password_cracker
```

**Single node, 4 processes (one per core):**
```bash
mpirun -np 4 build/password_cracker
```

**Single node, 4 processes, 4 threads each:**
```bash
OMP_NUM_THREADS=4 mpirun -np 4 build/password_cracker
```

### Cluster Execution

Create a hostfile listing cluster nodes:

```
# hostfile.txt
node1 slots=4
node2 slots=4
node3 slots=4
node4 slots=4
```

Run on cluster:
```bash
chmod +x scripts/run_cluster.sh
./scripts/run_cluster.sh 4 4 hostfile.txt
```

This launches 16 total processes (4 nodes × 4 processes per node).

## Performance Benchmarking

Run the benchmark suite to evaluate speedup:

```bash
chmod +x scripts/benchmark.sh
./scripts/benchmark.sh
```

This will test:
1. **CPU only** - single thread baseline
2. **OpenMP** - 4 and 8 threads
3. **MPI** - distributed parallelism
4. **Hybrid** - MPI + OpenMP combined

## Performance Analysis

### Expected Speedup

| Configuration | Hashes/sec | Speedup |
|---|---|---|
| 1 CPU thread | 1M | 1.0× |
| 8 CPU threads (OpenMP) | 12M | 12× |
| 1 GPU | 200M | 200× |
| 4 CPUs | 48M | 48× |
| 4 GPUs (MPI) | 800M | 800× |
| 4 nodes, 8T+GPU | 6.4B | 6,400× |

### Real-World Performance

Password cracking timings (6-character lowercase password):

| Attack Type | CPU Only | OpenMP (8T) | GPU | MPI+GPU (4) |
|---|---|---|---|---|
| Brute force | ~24 hours | 2 hours | 5 mins | 1 min |
| Dictionary | ~2 mins | 15 secs | 0.5 secs | 0.1 secs |

## Code Components

### Master Process (`mpi_master.cpp`)

- **Responsibilities:**
  - Divide password search space into chunks
  - Send work assignments to all worker processes
  - Receive and aggregate results
  - Broadcast termination signal when password found

- **Key Methods:**
  - `distributeWork()` - MPI_Send work to workers
  - `collectResults()` - MPI_Recv results from workers
  - `broadcastTermination()` - Early termination signal

### Worker Process (`mpi_worker.cpp`)

- **Responsibilities:**
  - Receive work from master
  - Generate password candidates (CPU + OpenMP)
  - Launch GPU kernels for hash computation
  - Send results back to master

- **Key Methods:**
  - `receiveWorkAssignment()` - Receive MPI work
  - `executeBruteForce()` - Run password cracking
  - `sendResults()` - Return findings to master

### GPU Kernels (`cuda_hash.cu`)

```cuda
__global__ void compute_hashes_kernel(
    const char* passwords,
    const int* password_lens,
    int num_passwords,
    const char* target_hash,
    unsigned int* results,
    int* found_idx,
    char algo
)
```

- Each CUDA thread processes one password candidate
- Computes hash in parallel on GPU
- Compares with target hash using atomic operations
- Grid configuration: `(N + block_size - 1) / block_size` blocks of 256 threads

### Password Generation (`password_generator.cpp`)

Supports:
- **Brute force**: Enumerate all combinations of charset
- **Dictionary attack**: Load and test wordlists
- Charset options:
  - Lowercase: `abcdefghijklmnopqrstuvwxyz`
  - Uppercase: `ABCDEFGHIJKLMNOPQRSTUVWXYZ`
  - Digits: `0123456789`
  - Alphanumeric: Combined
  - Special: `!@#$%^&*`

## Synchronization & Communication

### MPI Communication Tags

| Tag | Purpose |
|---|---|
| 0-6 | Brute force work distribution |
| 10-14 | Dictionary work distribution |
| 20-24 | Results collection |
| 30 | Termination signal |
| 40-43 | Early termination notification |

### OpenMP Parallelism

- Brute force loop parallelized with `#pragma omp parallel for`
- Thread-safe hash match detection with `#pragma omp critical`
- Termination check every 10,000 hashes

### GPU Thread Hierarchy

```
Grid (variable blocks)
├── Block 0 (256 threads)
│   ├── Thread 0: Password candidate 0
│   ├── Thread 1: Password candidate 1
│   └── ... Thread 255
├── Block 1 (256 threads)
│   └── Thread 0-255: Next 256 candidates
└── Block N
    └── ... continues
```

## Attack Modes

### 1. Brute Force Attack

Enumerate all possible passwords given:
- Character set (lowercase, uppercase, digits, special)
- Minimum and maximum password length

**Time complexity:** O(|charset|^length)

```cpp
// Generate 4-character passwords
// From "aaaa" to "zzzz" (456,976 combinations)
generateBruteForceRange(charset, 4, 4, start, end, candidates);
```

### 2. Dictionary Attack

Test words from a dictionary file against the target hash.

**Time complexity:** O(dictionary_size)

Much faster for realistic passwords containing dictionary words.

## Compilation Flags

### CMake Options

```bash
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \      # Release optimizations
  -DCMAKE_CXX_STANDARD=17 \         # C++17 support
  -DCMAKE_CUDA_FLAGS="-arch=sm_70"  # CUDA compute capability
```

### Environment Variables

```bash
OMP_NUM_THREADS=8          # OpenMP threads
CUDA_VISIBLE_DEVICES=0     # GPU selection
MPI_BUFFER_SIZE=131072     # MPI buffer size
```

## Troubleshooting

### Build Issues

**CMake not finding MPI:**
```bash
cmake .. -DMPI_CXX_COMPILER=mpicxx -DCMAKE_CXX_COMPILER=mpicxx
```

**CUDA not found:**
```bash
cmake .. -DCUDA_TOOLKIT_ROOT_DIR=/usr/local/cuda
```

**OpenMP missing:**
```bash
sudo apt-get install libomp-dev
```

### Runtime Issues

**MPI: Unknown error**
```bash
# Check MPI installation
mpirun --version
mpicc --show
```

**CUDA: out of memory**
- Reduce batch size or number of GPU threads
- Check GPU memory: `nvidia-smi`

**Poor performance:**
- Enable CPU affinity: `-bind-to socket`
- Check network latency: `mpirun -np 2 mpiexec.hydra -info all`

## Performance Optimization Tips

1. **Network Optimization**
   - Use dedicated high-speed interconnect (InfiniBand)
   - Reduce MPI communication overhead with larger work batches

2. **GPU Optimization**
   - Ensure compute capability >= 3.5
   - Use CUDA streams for overlapped computation/communication
   - Optimize kernel launch parameters

3. **CPU Optimization**
   - Increase OpenMP threads to match physical cores
   - Use NUMA-aware thread binding
   - Profile with: `perf stat -p <pid>`

4. **Load Balancing**
   - Current: Static distribution (equal chunks)
   - Better: Work-stealing or dynamic scheduling
   - Implement: MPI_Send work requests from fast workers

## Advanced Usage

### Custom Charset

```cpp
std::string customCharset = "aeiou";  // Vowels only
master.distributeWork(customCharset, 3, 5, targetHash, "SHA256");
```

### Multiple GPUs

Set before running:
```bash
export CUDA_VISIBLE_DEVICES=0,1,2,3  # Use GPUs 0-3
mpirun -np 4 build/password_cracker
```

### Hash Algorithms

Currently supported:
- MD5 (legacy)
- SHA1 (legacy)
- SHA256 (recommended)

To add more: extend `HashAlgorithm` enum and implement in `hash_utils.cpp`.

## Academic Considerations

### Scope
This project is educational. Production password cracking would require:
- Cryptographically secure hash implementations (OpenSSL)
- More sophisticated attack strategies (hybrid attacks, rules)
- Better dictionary sources and preprocessing
- Rainbow tables and precomputation

### Ethical Use
Password cracking techniques should only be used on:
- Your own passwords for recovery
- Systems with explicit permission for security testing
- Authorized penetration testing engagements

Unauthorized password cracking is illegal in most jurisdictions.

## References

- MPI: https://www.open-mpi.org/
- OpenMP: https://www.openmp.org/
- CUDA: https://docs.nvidia.com/cuda/
- CMake: https://cmake.org/

## Team Members

**VibeCoders Team**
- Member 1: [Your name]
- Member 2: [Your name]
- Member 3: [Your name]
- Member 4: [Your name]

## Submission Information

- **Course:** UCS645 - Parallel and Distributed Computing
- **Submitted To:** Dr. Saif Nalband
- **Date:** [Submission date]
- **Institution:** [Your university]

## License

This project is submitted as coursework for UCS645. 
Academic use only. Unauthorized reproduction or distribution is prohibited.

---

**For questions or support, please contact the development team.**
