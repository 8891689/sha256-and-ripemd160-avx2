// gcc -O3 -mavx2 -mfma -march=native -o ripemd160_test ripemd160_avx.c ripemd160_test.c
#include "ripemd160_avx.h" 
#include <stdio.h>          
#include <string.h>         
#include <stdlib.h>          
#include <time.h>            
#include <stdbool.h>        

// 對齊的 malloc
void* aligned_malloc(size_t size, size_t alignment) {
    void* p1; 
    void** p2; 
    int offset = alignment - 1 + sizeof(void*);
    if ((p1 = (void*)malloc(size + offset)) == NULL) {
       return NULL;
    }
    p2 = (void**)(((size_t)(p1) + offset) & ~(alignment - 1));
    p2[-1] = p1;
    return p2;
}

void aligned_free(void* p) {
    if (p == NULL) return;
    void* p1 = ((void**)p)[-1];
    free(p1);
}


int main() {
    RIPEMD160_MULTI_CTX ctx_empty; 
    uint8_t output_digests_empty[LANE_COUNT][DIGEST_SIZE];

    ripemd160_multi_init(&ctx_empty);
    ripemd160_multi_final(&ctx_empty, output_digests_empty);

    const char* expected_empty_hash_hex = "9c1185a5c5e9fc54612808977ee8f548b2258d31";
    uint8_t expected_empty_hash[DIGEST_SIZE];
    int i_scanf; 
    for(i_scanf=0; i_scanf < DIGEST_SIZE; ++i_scanf) {
        if (sscanf(expected_empty_hash_hex + 2*i_scanf, "%2hhx", &expected_empty_hash[i_scanf]) != 1) {
            fprintf(stderr, "Error parsing expected_empty_hash_hex\n");
            return 1;
        }
    }


    printf("--- Testing RIPEMD-160 (%d-Lane Parallel, C Main, C++ Core) ---\n", LANE_COUNT);
    printf("Test Case: Empty Input\n");
    bool all_empty_ok = true;
    for (int lane = 0; lane < LANE_COUNT; ++lane) {
        printf("  Lane %d Hash: ", lane);
        for (int i = 0; i < DIGEST_SIZE; ++i) {
            printf("%02x", output_digests_empty[lane][i]);
        }
        if (memcmp(output_digests_empty[lane], expected_empty_hash, DIGEST_SIZE) != 0) {
            printf(" FAIL (Expected: %s)", expected_empty_hash_hex);
            all_empty_ok = false;
        } else {
            printf(" OK");
        }
        printf("\n");
    }
    if (!all_empty_ok) {
        fprintf(stderr, "!!! EMPTY STRING TEST FAILED FOR ONE OR MORE LANES !!!\n");
    }
    printf("------------------------------------------\n");

    // --- Test Case: "abc" ---
    RIPEMD160_MULTI_CTX ctx_abc;
    uint8_t output_digests_abc[LANE_COUNT][DIGEST_SIZE];
    const char* abc_input_str = "abc";
    size_t abc_input_len = strlen(abc_input_str);

    ripemd160_multi_init(&ctx_abc);

    for (int lane = 0; lane < LANE_COUNT; ++lane) {
        memcpy(ctx_abc.buffer[lane], abc_input_str, abc_input_len);
        ctx_abc.buffer_len[lane] = (uint32_t)abc_input_len; 
    }
    ripemd160_multi_final(&ctx_abc, output_digests_abc);

    const char* expected_abc_hash_hex = "8eb208f7e05d987a9b044a8e98c6b087f15a0bfc";
    uint8_t expected_abc_hash[DIGEST_SIZE];
    for(i_scanf=0; i_scanf < DIGEST_SIZE; ++i_scanf) {
         if (sscanf(expected_abc_hash_hex + 2*i_scanf, "%2hhx", &expected_abc_hash[i_scanf]) != 1) {
            fprintf(stderr, "Error parsing expected_abc_hash_hex\n");
            return 1;
        }
    }

    printf("\nTest Case: Input \"abc\"\n");
    bool all_abc_ok = true;
    for (int lane = 0; lane < LANE_COUNT; ++lane) {
        printf("  Lane %d Hash: ", lane);
        for (int i = 0; i < DIGEST_SIZE; ++i) {
            printf("%02x", output_digests_abc[lane][i]);
        }
        if (memcmp(output_digests_abc[lane], expected_abc_hash, DIGEST_SIZE) != 0) {
            printf(" FAIL (Expected: %s)", expected_abc_hash_hex);
            all_abc_ok = false;
        } else {
            printf(" OK");
        }
        printf("\n");
    }
     if (!all_abc_ok) {
        fprintf(stderr, "!!! \"abc\" STRING TEST FAILED FOR ONE OR MORE LANES !!!\n");
    }
    printf("------------------------------------------\n");

    // --- Test Case: Single Block of 64 Zeros ---
    RIPEMD160_MULTI_CTX ctx_zeros;
    uint8_t input_blocks_zeros[LANE_COUNT][BLOCK_SIZE];
    uint8_t output_digests_zeros[LANE_COUNT][DIGEST_SIZE];

    memset(input_blocks_zeros, 0, sizeof(input_blocks_zeros));
    ripemd160_multi_init(&ctx_zeros);
    ripemd160_multi_update_full_blocks(&ctx_zeros, input_blocks_zeros);
    ripemd160_multi_final(&ctx_zeros, output_digests_zeros);

    const char* expected_zero_block_hash_hex = "9b8ccc2f374ae313a914763cc9cdfb47bfe1c229";
    uint8_t expected_zero_block_hash[DIGEST_SIZE];
    for(i_scanf=0; i_scanf < DIGEST_SIZE; ++i_scanf) {
        if (sscanf(expected_zero_block_hash_hex + 2*i_scanf, "%2hhx", &expected_zero_block_hash[i_scanf]) != 1) {
            fprintf(stderr, "Error parsing expected_zero_block_hash_hex\n");
            return 1;
        }
    }

    printf("\nTest Case: Single Block of 64 Zeros\n");
    bool all_zeros_ok = true;
    for (int lane = 0; lane < LANE_COUNT; ++lane) {
        printf("  Lane %d Hash: ", lane);
        for (int i = 0; i < DIGEST_SIZE; ++i) {
            printf("%02x", output_digests_zeros[lane][i]);
        }
        if (memcmp(output_digests_zeros[lane], expected_zero_block_hash, DIGEST_SIZE) != 0) {
            printf(" FAIL (Expected: %s)", expected_zero_block_hash_hex);
            all_zeros_ok = false;
        } else {
            printf(" OK");
        }
        printf("\n");
    }
    if (!all_zeros_ok) {
        fprintf(stderr, "!!! 64 ZEROS BLOCK TEST FAILED FOR ONE OR MORE LANES !!!\n");
    }
    printf("------------------------------------------\n");

    // --- Test Case: Message of 56 'a' characters ---
    RIPEMD160_MULTI_CTX ctx_56a;
    uint8_t output_digests_56a[LANE_COUNT][DIGEST_SIZE];
    const int len_56a = 56;
    char input_56a[len_56a]; 
    memset(input_56a, 'a', len_56a);

    ripemd160_multi_init(&ctx_56a);
    for (int lane = 0; lane < LANE_COUNT; ++lane) {
        memcpy(ctx_56a.buffer[lane], input_56a, len_56a);
        ctx_56a.buffer_len[lane] = len_56a;
    }
    ripemd160_multi_final(&ctx_56a, output_digests_56a);

    const char* expected_56a_hash_hex = "e72334b46c83cc70bef979e15453706c95b888be";
    uint8_t expected_56a_hash[DIGEST_SIZE];
     for(i_scanf=0; i_scanf < DIGEST_SIZE; ++i_scanf) {
        if (sscanf(expected_56a_hash_hex + 2*i_scanf, "%2hhx", &expected_56a_hash[i_scanf]) != 1) {
            fprintf(stderr, "Error parsing expected_56a_hash_hex\n");
            return 1;
        }
    }

    printf("\nTest Case: Input 56 'a's (tests 2 padding blocks)\n");
    bool all_56a_ok = true;
    for (int lane = 0; lane < LANE_COUNT; ++lane) {
        printf("  Lane %d Hash: ", lane);
        for (int i = 0; i < DIGEST_SIZE; ++i) {
            printf("%02x", output_digests_56a[lane][i]);
        }
        if (memcmp(output_digests_56a[lane], expected_56a_hash, DIGEST_SIZE) != 0) {
            printf(" FAIL (Expected: %s)", expected_56a_hash_hex);
            all_56a_ok = false;
        } else {
            printf(" OK");
        }
        printf("\n");
    }
    if (!all_56a_ok) {
        fprintf(stderr, "!!! 56 'a's TEST FAILED FOR ONE OR MORE LANES !!!\n");
    }
    printf("------------------------------------------\n");


    // --- Performance Test ---
    size_t data_size_per_lane_bytes = 128 * 1024 * 1024;
    unsigned long long total_mem_for_lanes = (unsigned long long)LANE_COUNT * data_size_per_lane_bytes;
    unsigned long long max_total_data_for_perf_blocks = 2ULL * 1024 * 1024 * 1024;


    if (total_mem_for_lanes > max_total_data_for_perf_blocks / BLOCK_SIZE * BLOCK_SIZE ) { 
        data_size_per_lane_bytes = (max_total_data_for_perf_blocks / LANE_COUNT);
    }

    data_size_per_lane_bytes = (data_size_per_lane_bytes / BLOCK_SIZE) * BLOCK_SIZE;
    if (data_size_per_lane_bytes == 0 && LANE_COUNT > 0) data_size_per_lane_bytes = BLOCK_SIZE;


    size_t num_blocks_per_lane = data_size_per_lane_bytes / BLOCK_SIZE;
    unsigned long long total_data_for_perf_test = (unsigned long long)LANE_COUNT * data_size_per_lane_bytes;


    printf("\n--- Parallel Performance Test ---\n");
    printf("Data per lane: %.2f MiB\n", (double)data_size_per_lane_bytes / (1024.0 * 1024.0));
    printf("Total data for test: %.2f GiB\n", (double)total_data_for_perf_test / (1024.0 * 1024.0 * 1024.0));

    if (num_blocks_per_lane > 0) {
        printf("Processing %zu blocks per lane.\n", num_blocks_per_lane);

        uint8_t (*perf_input_blocks)[BLOCK_SIZE];
        perf_input_blocks = (uint8_t (*)[BLOCK_SIZE])aligned_malloc(LANE_COUNT * BLOCK_SIZE, 64);

        if (perf_input_blocks == NULL) {
            fprintf(stderr, "Memory allocation failed for perf_input_blocks\n");
            return 1;
        }


        for (int lane = 0; lane < LANE_COUNT; ++lane) {
            for (int i = 0; i < BLOCK_SIZE; ++i) {
                perf_input_blocks[lane][i] = (uint8_t)((lane + i + (i*7)) % 256);
            }
        }

        RIPEMD160_MULTI_CTX perf_ctx; 
        uint8_t perf_output_digests[LANE_COUNT][DIGEST_SIZE];

        ripemd160_multi_init(&perf_ctx);

        clock_t start_time = clock();

        for (size_t block_idx = 0; block_idx < num_blocks_per_lane; ++block_idx) {
            if ((block_idx & 0xFFF) == 0) {
                 for(int lane_idx = 0; lane_idx < LANE_COUNT; ++lane_idx) {
                    perf_input_blocks[lane_idx][0] = (uint8_t)(block_idx + lane_idx);
                 }
            }
            ripemd160_multi_update_full_blocks(&perf_ctx, perf_input_blocks);
        }
        ripemd160_multi_final(&perf_ctx, perf_output_digests);

        clock_t end_time = clock();
        double elapsed_seconds = (double)(end_time - start_time) / CLOCKS_PER_SEC;

        aligned_free(perf_input_blocks);

        if (elapsed_seconds > 0.000001) {
            double throughput_mib_s = ((double)total_data_for_perf_test / (1024.0 * 1024.0)) / elapsed_seconds;
            double throughput_gb_s = throughput_mib_s / 1024.0;

            printf("Time taken: %.3f seconds.\n", elapsed_seconds);
            printf("Total data processed: %.2f MiB.\n", (double)total_data_for_perf_test / (1024.0*1024.0));
            printf("Aggregate throughput: %.2f MiB/s (%.2f GiB/s).\n", throughput_mib_s, throughput_gb_s);
            printf("Sample Perf Test Digest (Lane 0): ");
            for (int i = 0; i < DIGEST_SIZE; ++i) {
                printf("%02x", perf_output_digests[0][i]);
            }
            printf("\n");
        } else {
            printf("Time taken (%.6fs) was too short to measure reliably. Increase data size.\n", elapsed_seconds);
        }
    } else {
        printf("Performance test skipped: data size per lane is zero.\n");
    }
    printf("-----------------------------------\n");

    return 0;
}
