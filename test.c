// test.c  gcc test.c sha256_avx.c -o sha256_test -O3 -mavx2 -march=native -Wall
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "sha256_avx.h"

// 準備一個簡單的測試數據塊 (SHA-256 的標準填充)
void prepare_test_data_block(uint8_t block[64], const char* message, uint64_t message_len_bytes) {
    memset(block, 0, 64);
    if (message && message_len_bytes > 0) {
        memcpy(block, message, message_len_bytes);
    }
    block[message_len_bytes] = 0x80; // 添加填充位
    // 對於只有一個塊的消息，長度放在最後 8 bytes
    if (message_len_bytes < 56) {
        uint64_t bit_length = __builtin_bswap64(message_len_bytes * 8); // 轉換為大端序
        memcpy(block + 56, &bit_length, 8);
    }
}

// 打印一個 AVX8 上下文的狀態，用於驗證
void print_hash_results(SHA256_CTX_AVX8* ctx, const char* label) {
    printf("--- %s ---\n", label);
    uint32_t final_hash[8][8];
    // 將 SoA 數據轉換回 AoS 以便打印
    _mm256_store_si256((__m256i*)final_hash[0], ctx->state[0]);
    _mm256_store_si256((__m256i*)final_hash[1], ctx->state[1]);
    _mm256_store_si256((__m256i*)final_hash[2], ctx->state[2]);
    _mm256_store_si256((__m256i*)final_hash[3], ctx->state[3]);
    _mm256_store_si256((__m256i*)final_hash[4], ctx->state[4]);
    _mm256_store_si256((__m256i*)final_hash[5], ctx->state[5]);
    _mm256_store_si256((__m256i*)final_hash[6], ctx->state[6]);
    _mm256_store_si256((__m256i*)final_hash[7], ctx->state[7]);

    for(int block_idx = 0; block_idx < 8; ++block_idx) {
        printf("Block %d: ", block_idx);
        for(int i = 0; i < 8; ++i) {
            printf("%08x", final_hash[i][block_idx]);
        }
        printf("\n");
    }
    printf("Expected for 'abc': ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad\n");
}


int main() {
    // --- 1. 正確性驗證 ---
    printf("--- Correctness Test ---\n");
    SHA256_CTX_AVX8 ctx __attribute__((aligned(64)));
    uint8_t input_blocks[8][64] __attribute__((aligned(64)));

    // 準備 8 個相同的數據塊 ("abc")
    for(int i = 0; i < 8; ++i) {
        prepare_test_data_block(input_blocks[i], "abc", 3);
    }

    init_ctx_avx8_for_benchmark(&ctx);
    sha256_transform_avx8(&ctx, input_blocks);
    print_hash_results(&ctx, "Final Hashes (Optimized Version)");
    printf("\n");


    // --- 2. 性能測試 ---
    printf("--- Performance Benchmark ---\n");
    const long long NUM_ITERATIONS = 3000000; // 迭代次數可以根據機器性能調整
    const int BLOCKS_PER_CALL = 8;
    
    // 初始化上下文
    init_ctx_avx8_for_benchmark(&ctx);

    // 預熱，避免 JIT 或緩存效應影響計時
    for (int i=0; i < 1000; ++i) {
        sha256_transform_avx8(&ctx, input_blocks);
    }

    // 重新初始化，開始正式測試
    init_ctx_avx8_for_benchmark(&ctx);

    clock_t start = clock();
    for(long long i = 0; i < NUM_ITERATIONS; ++i) {
        sha256_transform_avx8(&ctx, input_blocks);
    }
    clock_t end = clock();

    double total_time = (double)(end - start) / CLOCKS_PER_SEC;
    long long total_hashes = NUM_ITERATIONS * BLOCKS_PER_CALL;
    double hashes_per_sec = (double)total_hashes / total_time;
    double gigabytes_per_sec = (hashes_per_sec * 64) / (1024 * 1024 * 1024);

    printf("Total hashes processed: %lld\n", total_hashes);
    printf("Total time: %.4f seconds\n", total_time);
    printf("Performance: %.2f Million Hashes/sec\n", hashes_per_sec / 1e6);
    printf("Throughput:  %.2f GB/s\n", gigabytes_per_sec);

    return 0;
}
