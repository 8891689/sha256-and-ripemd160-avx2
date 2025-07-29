# Introduction: High-performance parallel SHA-256 and ripemd160 implementation based on AVX2 c/c++

This is a SHA-256 and ripemd160 implementation optimized for modern x86-64 CPUs.

Its main goal is to maximize computational throughput.

It utilizes the AVX2 (Advanced Vector Extensions 2) instruction set to achieve parallel processing of 8 independent data blocks at the same time.

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
--- Correctness Test (Using C Wrapper with Diverse Inputs) ---
--- Verification Results ---
Test Case 0:
  Input:    ""
  Expected: e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
  Result:   e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
  Status:   PASS

Test Case 1:
  Input:    "abc"
  Expected: ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad
  Result:   ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad
  Status:   PASS

Test Case 2:
  Input:    "a"
  Expected: ca978112ca1bbdcafac231b39a23dc4da786eff8147c4e72b9807785afee48bb
  Result:   ca978112ca1bbdcafac231b39a23dc4da786eff8147c4e72b9807785afee48bb
  Status:   PASS

Test Case 3:
  Input:    "hello world"
  Expected: b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9
  Result:   b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9
  Status:   PASS

Test Case 4:
  Input:    "1234567890"
  Expected: c775e7b757ede630cd0aa1113bd102661ab38829ca52a6422ab782862f268646
  Result:   c775e7b757ede630cd0aa1113bd102661ab38829ca52a6422ab782862f268646
  Status:   PASS

Test Case 5:
  Input:    "SHA256 AVX2 Test"
  Expected: 19f76accdedb73f6333c275e79cf840de7ec75f5e4ae18399f81830dd859ec36
  Result:   19f76accdedb73f6333c275e79cf840de7ec75f5e4ae18399f81830dd859ec36
  Status:   PASS

Test Case 6:
  Input:    "一生二，二生三，三生万物"
  Expected: 2c7b572cc2a7a3c0af7d2946f79973f15f3f56fd6fc6d85f2d2b5c8d272aa3ed
  Result:   2c7b572cc2a7a3c0af7d2946f79973f15f3f56fd6fc6d85f2d2b5c8d272aa3ed
  Status:   PASS

Test Case 7:
  Input:    "https://github.com/8891689"
  Expected: 73f9f4d7581f581a8be499fd002160182f4953ab0a736494540161395fb94e81
  Result:   73f9f4d7581f581a8be499fd002160182f4953ab0a736494540161395fb94e81
  Status:   PASS

--- Summary ---
All 8 tests passed successfully!

--- Performance Benchmark (Using C Wrapper) ---
Total hashes processed: 40000000
Total time: 2.6321 seconds
Performance: 15.20 Million Hashes/sec
Throughput:  0.91 GB/s




 ./ripemd160_test
--- Test RIPEMD-160 (8 channels in parallel, C/C++ core) ---
Test Case: Empty Input
  Lane 0 Hash: 9c1185a5c5e9fc54612808977ee8f548b2258d31 OK
  Lane 1 Hash: 9c1185a5c5e9fc54612808977ee8f548b2258d31 OK
  Lane 2 Hash: 9c1185a5c5e9fc54612808977ee8f548b2258d31 OK
  Lane 3 Hash: 9c1185a5c5e9fc54612808977ee8f548b2258d31 OK
  Lane 4 Hash: 9c1185a5c5e9fc54612808977ee8f548b2258d31 OK
  Lane 5 Hash: 9c1185a5c5e9fc54612808977ee8f548b2258d31 OK
  Lane 6 Hash: 9c1185a5c5e9fc54612808977ee8f548b2258d31 OK
  Lane 7 Hash: 9c1185a5c5e9fc54612808977ee8f548b2258d31 OK
------------------------------------------

Test Case: Input "abc"
  Lane 0 Hash: 8eb208f7e05d987a9b044a8e98c6b087f15a0bfc OK
  Lane 1 Hash: 8eb208f7e05d987a9b044a8e98c6b087f15a0bfc OK
  Lane 2 Hash: 8eb208f7e05d987a9b044a8e98c6b087f15a0bfc OK
  Lane 3 Hash: 8eb208f7e05d987a9b044a8e98c6b087f15a0bfc OK
  Lane 4 Hash: 8eb208f7e05d987a9b044a8e98c6b087f15a0bfc OK
  Lane 5 Hash: 8eb208f7e05d987a9b044a8e98c6b087f15a0bfc OK
  Lane 6 Hash: 8eb208f7e05d987a9b044a8e98c6b087f15a0bfc OK
  Lane 7 Hash: 8eb208f7e05d987a9b044a8e98c6b087f15a0bfc OK
------------------------------------------

Test Case: Single Block of 64 Zeros
  Lane 0 Hash: 9b8ccc2f374ae313a914763cc9cdfb47bfe1c229 OK
  Lane 1 Hash: 9b8ccc2f374ae313a914763cc9cdfb47bfe1c229 OK
  Lane 2 Hash: 9b8ccc2f374ae313a914763cc9cdfb47bfe1c229 OK
  Lane 3 Hash: 9b8ccc2f374ae313a914763cc9cdfb47bfe1c229 OK
  Lane 4 Hash: 9b8ccc2f374ae313a914763cc9cdfb47bfe1c229 OK
  Lane 5 Hash: 9b8ccc2f374ae313a914763cc9cdfb47bfe1c229 OK
  Lane 6 Hash: 9b8ccc2f374ae313a914763cc9cdfb47bfe1c229 OK
  Lane 7 Hash: 9b8ccc2f374ae313a914763cc9cdfb47bfe1c229 OK
------------------------------------------

Test Case: Input 56 'a's (tests 2 padding blocks)
  Lane 0 Hash: e72334b46c83cc70bef979e15453706c95b888be OK
  Lane 1 Hash: e72334b46c83cc70bef979e15453706c95b888be OK
  Lane 2 Hash: e72334b46c83cc70bef979e15453706c95b888be OK
  Lane 3 Hash: e72334b46c83cc70bef979e15453706c95b888be OK
  Lane 4 Hash: e72334b46c83cc70bef979e15453706c95b888be OK
  Lane 5 Hash: e72334b46c83cc70bef979e15453706c95b888be OK
  Lane 6 Hash: e72334b46c83cc70bef979e15453706c95b888be OK
  Lane 7 Hash: e72334b46c83cc70bef979e15453706c95b888be OK
------------------------------------------

--- Parallel Performance Test ---
Data per lane: 128.00 MiB
Total data for test: 1.00 GiB
Processing 2097152 blocks per lane.
Time taken: 0.907 seconds.
Total data processed: 1024.00 MiB.
Aggregate throughput: 1129.53 MiB/s (1.10 GiB/s).
Sample Perf Test Digest (Lane 0): c8a729420cdf71959d50d24a3ce9eff2cf540290


./main_full_test
Starting 100000 HASH160 calculations using PURE AVX2 pipeline...
  - Both key types handled by multi-block AVX2-SHA256 -> AVX2-RIPEMD160.
Processing in batches of 8. Results for the last 5 public keys will be printed.


--- Results from the final batch (Verification) ---
--- Public Key Index: 99996 ---
  Private Key:             000000000000000000000000000000000000000000000000000000000001869c
  HASH160 (Comp, Pure AVX):6d7eec82c07359cdb24d70c6fe1ac3454c2bd60c
  HASH160 (Comp, OpenSSL): 6d7eec82c07359cdb24d70c6fe1ac3454c2bd60c
  (Comp Verified OK)
  HASH160 (Uncomp, Pure AVX):8799dd474bc40898c3edc50f53b79fa0faca03b9
  HASH160 (Uncomp, OpenSSL):8799dd474bc40898c3edc50f53b79fa0faca03b9
  (Uncomp Verified OK)

--- Public Key Index: 99997 ---
  Private Key:             000000000000000000000000000000000000000000000000000000000001869d
  HASH160 (Comp, Pure AVX):4bc50155d2a8876b30115bb1ea1c5bfe760fa2de
  HASH160 (Comp, OpenSSL): 4bc50155d2a8876b30115bb1ea1c5bfe760fa2de
  (Comp Verified OK)
  HASH160 (Uncomp, Pure AVX):9da5bc952dc01bfc597faf43957877890908905f
  HASH160 (Uncomp, OpenSSL):9da5bc952dc01bfc597faf43957877890908905f
  (Uncomp Verified OK)

--- Public Key Index: 99998 ---
  Private Key:             000000000000000000000000000000000000000000000000000000000001869e
  HASH160 (Comp, Pure AVX):d67e742929b38ae8268732fba15cf4b9b6899f9e
  HASH160 (Comp, OpenSSL): d67e742929b38ae8268732fba15cf4b9b6899f9e
  (Comp Verified OK)
  HASH160 (Uncomp, Pure AVX):4ba78b9af152b6f559d149294e7d8be9746d4ed4
  HASH160 (Uncomp, OpenSSL):4ba78b9af152b6f559d149294e7d8be9746d4ed4
  (Uncomp Verified OK)

--- Public Key Index: 99999 ---
  Private Key:             000000000000000000000000000000000000000000000000000000000001869f
  HASH160 (Comp, Pure AVX):b58ef9b3d0292ee25c3aeb563826a9912f454c0e
  HASH160 (Comp, OpenSSL): b58ef9b3d0292ee25c3aeb563826a9912f454c0e
  (Comp Verified OK)
  HASH160 (Uncomp, Pure AVX):09a6c8f0208d6ec7905a0fbac4f17445fd55272e
  HASH160 (Uncomp, OpenSSL):09a6c8f0208d6ec7905a0fbac4f17445fd55272e
  (Uncomp Verified OK)

--- Public Key Index: 100000 ---
  Private Key:             00000000000000000000000000000000000000000000000000000000000186a0
  HASH160 (Comp, Pure AVX):96a9d76a7f3f961e3082c13be70d09190e4fc34b
  HASH160 (Comp, OpenSSL): 96a9d76a7f3f961e3082c13be70d09190e4fc34b
  (Comp Verified OK)
  HASH160 (Uncomp, Pure AVX):96bcc760ca75180fd7fc83ede307897f4bb86568
  HASH160 (Uncomp, OpenSSL):96bcc760ca75180fd7fc83ede307897f4bb86568
  (Uncomp Verified OK)

--- Performance Summary (Pure AVX2 Pipeline) ---
Total public keys processed: 100000
Total HASH160s computed:     200000 (2 per pubkey)
Total time:                  3.0738 seconds
Performance:                 65066.11 HASH160s/sec
Performance:                 0.07 Million HASH160s/sec


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

