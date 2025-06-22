# Introduction: A High-Performance Parallel SHA-256 Implementation with AVX2

This is a SHA-256 implementation optimized for modern x86-64 CPUs。

with the primary goal of maximizing computational throughput. 

It leverages the AVX2 (Advanced Vector Extensions 2) instruction set to achieve the parallel processing of 8 independent data blocks simultaneously.

# Core Technical Features:

1. 8-Way Parallelism:
By utilizing AVX2's 256-bit vector registers, each capable of holding eight 32-bit integers。 

the design allows every SHA-256 operation (such as addition, XOR, and rotation) to be applied to 8 separate hash computation streams at the same time.

2. SoA (Structure of Arrays) Data Layout:
To facilitate vectorized computation, the data layout is transformed from the traditional "hash states one after another" (AoS) to a "structure of arrays" (SoA) format。

where all 'a' states are grouped together, all 'b' states are grouped together, and so on. This layout is crucial for achieving high-performance parallel processing.

Efficient Data Transposition:
The code includes a highly optimized transposition module. 

It uses a sequence of unpack, permute, and shuffle instructions to rapidly convert 8 independent input data blocks (64 bytes each) into the SoA format required for parallel computation。

efficiently preparing the data for the core compression loop.

3. Optimized Compression Loop:
2-Way Loop Unrolling: The main loop processes two rounds (i and i+1) per iteration. 

This moderate level of unrolling provides the CPU's out-of-order execution engine with sufficient independent instructions to hide latency。

while avoiding the instruction cache and register pressure issues caused by excessive unrolling.

Constant Pre-loading: Before the main computation begins, all required SHA-256 K constants are pre-loaded and broadcast into vector registers. 

This eliminates the overhead of repeatedly generating these constants inside the critical loop.

4. Performance:
On a modern CPU equipped with AVX2 and compiled with optimization flags such as -O3 -march=native。

this implementation can achieve an exceptional performance of over 15 million SHA-256 block computations per second。

with a data throughput approaching 1 GB/s.

Based on Intel® Xeon® E5-2697 v4 2.30 GHz single-threaded environment

```
./sha256_test
--- Correctness Test ---
--- Final Hashes (Optimized Version) ---
Block 0: ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad
Block 1: ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad
Block 2: ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad
Block 3: ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad
Block 4: ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad
Block 5: ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad
Block 6: ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad
Block 7: ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad
Expected for 'abc': ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad

--- Performance Benchmark ---
Total hashes processed: 24000000
Total time: 1.5665 seconds
Performance: 15.32 Million Hashes/sec
Throughput:  0.91 GB/s
```


### Sponsorship
If this project has been helpful to you, please consider sponsoring. Your support is greatly appreciated. Thank you!
```
BTC: bc1qt3nh2e6gjsfkfacnkglt5uqghzvlrr6jahyj2k
ETH: 0xD6503e5994bF46052338a9286Bc43bC1c3811Fa1
DOGE: DTszb9cPALbG9ESNJMFJt4ECqWGRCgucky
TRX: TAHUmjyzg7B3Nndv264zWYUhQ9HUmX4Xu4
```
### 📜 Disclaimer

This tool is provided for learning and research purposes only. Please use it with an understanding of the relevant risks. 

The developers are not responsible for financial losses or legal liability -caused by the use of this tool.

