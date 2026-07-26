# Parallel Numerical Integration: Pthreads, OpenMP & CUDA

[![Language: C/C++](https://img.shields.io/badge/Language-C%2FC%2B%2B-blue.svg)](https://en.cppreference.com/)
[![API: POSIX Threads](https://img.shields.io/badge/API-Pthreads-green.svg)](https://pubs.opengroup.org/onlinepubs/9699919799/)
[![API: OpenMP](https://img.shields.io/badge/API-OpenMP-orange.svg)](https://www.openmp.org/)
[![API: NVIDIA CUDA](https://img.shields.io/badge/API-CUDA_12.0-76B900.svg)](https://developer.nvidia.com/cuda-toolkit)
[![Course: Parallel Programming](https://img.shields.io/badge/AUEB-Parallel_Programming_2025--26-red.svg)](https://www.dept.aueb.gr/cs)

## Overview

This repository contains a comprehensive benchmark and implementation suite for **Parallel Numerical Integration** using the **Trapezoidal Rule**. Developed as part of the *Parallel Programming (2025–2026)* course at the **Athens University of Economics and Business (AUEB)**, Department of Informatics.

The project explores multi-core CPU and massive GPU parallel computing architectures across three primary parallel programming models:
1. **POSIX Threads (Pthreads)** — Shared memory multi-threading with manual synchronization, static work splitting, cyclic jump scheduling, and thread-safe work queues.
2. **OpenMP** — Compiler-directive based parallelization covering static/dynamic/guided loop scheduling, reductions, and explicit asynchronous tasking (linear & recursive divide-and-conquer).
3. **NVIDIA CUDA** — High-performance GPU acceleration, exploring memory hierarchy optimizations (Global, Registers, Shared Memory with Parallel Tree Reduction), grid-stride loops, and warp divergence mitigation.

---

## Mathematical Background

The numerical approximation of the definite integral $\int_{a}^{b} f(x) dx$ is computed using the **Trapezoidal Rule**:

$$\int_{a}^{b} f(x) dx \approx \frac{h}{2} \left[ f(a) + f(b) + 2 \sum_{i=1}^{n-1} f(a + i \cdot h) \right], \quad \text{where } h = \frac{b - a}{n}$$

To evaluate load balancing and execution characteristics under varied computational intensities, two workloads are tested:
- **Regular Workload**: $f(x) = x^2$ over $[0, 10]$ (uniform computational effort per iteration).
- **Irregular / Dynamic Workload**: Synthetic trigonometric functions $f(x)$ with variable iteration depth $g(x) = \lfloor |\sin(x \cdot 123.45)| 	imes 600 
floor$ creating predictable or pseudo-random computational spikes (simulating severe load imbalance and warp divergence).

---

## Key Implementations & Approaches

### Assignment 1: POSIX Threads (Pthreads)
- **Part A1 — Static Chunking with Mutex Updates**: Equal slice static partitioning ($N / 	ext{num\_threads}$); periodic critical-section accumulation with progressive integral output using `pthread_mutex_t`.
- **Part A2 — Static Chunking with Local Accumulation**: Thread-isolated arrays storing partial sums; zero mutex contention during execution and final accumulation in the main thread (`pthread_join`).
- **Part B — Cyclic / Jump Scheduling**: Interleaved loop step ($i \mathrel{+}= 	ext{num\_threads}$) ensuring balanced distribution of light vs. computationally heavy iterations.
- **Part C — Dynamic Work Queue**: Central thread-safe task counter protected by mutex; threads fetch dynamic variable-sized chunks (`CHUNK_SIZE`) to mitigate load imbalance.

### Assignment 2: OpenMP Parallelization
- **Part A — Work-Sharing For Loops**: Comparison between `#pragma omp parallel for reduction(+:sum)` vs. thread-local accumulation with `#pragma omp atomic`.
- **Part B — Loop Scheduling Policies**: Performance study of `schedule(static)`, `schedule(dynamic, chunk)`, and `schedule(guided, chunk)` under varying chunk sizes (1 to 100,000,000).
- **Part C — OpenMP Explicit Tasks**:
  - *Linear Task Generation*: Single master thread packaging chunks into `#pragma omp task`.
  - *Recursive Divide-and-Conquer Tasking*: Binary sub-interval decomposition with `#pragma omp taskwait` and explicit recursion depth thresholding ($N \le 1000$) to prevent task-creation overhead.

### Assignment 3: GPU Acceleration with NVIDIA CUDA
- **Memory Hierarchy Benchmarking**:
  - *Naïve / Global Memory*: Direct atomic additions (`atomicAdd`) to global memory on every iteration (severe bottleneck).
  - *Registers + Grid-Stride Loop*: Thread-local register accumulation before a single global `atomicAdd`.
  - *Shared Memory + Parallel Tree Reduction*: Block-level dynamic shared memory array (`sdata[]`), intra-block `__syncthreads()` synchronization, logarithmic tree reduction, and single atomic commit per block by `threadIdx.x == 0`.
- **Warp Divergence Analysis**: Evaluating performance degradation when threads in a 32-thread warp experience branch execution variance under non-uniform workloads.

---

## Benchmark Results Summary

Experiments were conducted on **AMD Ryzen AI 7 350** (8 Cores / 16 Processors) with an **NVIDIA GeForce RTX 5060 Laptop GPU** (26 SMs, Compute Capability 12.0) on Windows 11 (MSYS2 MinGW-w64).

### Performance Comparison ($N = 100,000,000$, Regular Function $f(x) = x^2$)

| Programming Paradigm | Best Strategy / Configuration | Execution Time (sec) | Speedup vs CPU Serial |
| :--- | :--- | :---: | :---: |
| **CPU Serial** | Standard sequential `for` loop | `0.3630 s` | 1.0x |
| **GPU Serial** | CUDA Kernel `<<<1, 1>>>` | `0.1690 s` | 2.1x |
| **POSIX Threads** | Part A2: Thread-Local Array (8 threads) | `0.1150 s` | 3.16x |
| **OpenMP** | Work-sharing `reduction(+:sum)` (8 threads) | `0.0440 s` | 8.25x |
| **NVIDIA CUDA** | Registers / Shared Memory Tree Reduction | **`0.0072 s`** | **50.41x** |

> **Key Takeaway**: CUDA Shared Memory / Register implementations achieved a **50x speedup** over single-threaded execution and **6x speedup** over the fastest multi-core CPU OpenMP implementation.

---

## Build & Execution Instructions

### Prerequisites
- **C/C++ Compiler**: `gcc` / `g++` (v14+) with POSIX threads (`-pthread`) and OpenMP (`-fopenmp`) support.
- **CUDA Toolkit**: NVIDIA CUDA Toolkit 12.x (`nvcc`).
- **Environment**: Linux or Windows (MSYS2 MinGW-w64 environment recommended).

### Compiling and Running

#### 1. POSIX Threads (Pthreads)
```bash
# Compile Pthreads implementation (No optimization flags as per assignment requirements)
gcc -Wall -pthread 1_Pthreads/src/partA_static_nolocks.c -o pthreads_sim -lm

# Run executable
./pthreads_sim
```

#### 2. OpenMP
```bash
# Compile OpenMP implementation
gcc -Wall -fopenmp 2_OpenMP/src/integration_openmp_schedules.c -o openmp_sim -lm

# Set number of threads and run
export OMP_NUM_THREADS=8
./openmp_sim
```

#### 3. NVIDIA CUDA
```bash
# Compile CUDA Kernel using nvcc
nvcc -O2 3_CUDA/src/integration_cuda.cu -o cuda_sim

# Run GPU simulation
./cuda_sim
```
