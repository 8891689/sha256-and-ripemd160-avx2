/* sha256_test.c   
 * gcc -O3 -mavx2 -march=native sha256_test.c sha256_avx.c -o sha256_test 
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdalign.h>

#include "sha256_avx.h" 


// Define a test case structure
typedef struct {
    const char* input;
    const char* expected_hash;
} TestCase;

// Helper functions for printing and verifying results
void verify_and_print_results(const TestCase test_cases[8], const uint8_t final_hashes[8][32]) {
    printf("--- Verification Results ---\n");
    int failed_tests = 0;
    char calculated_hex[65]; 

    for (int i = 0; i < 8; ++i) {
        printf("Test Case %d:\n", i);
        printf("  Input:    \"%s\"\n", test_cases[i].input);
        printf("  Expected: %s\n", test_cases[i].expected_hash);

        for (int j = 0; j < 32; ++j) {
            sprintf(calculated_hex + j * 2, "%02x", final_hashes[i][j]);
        }
        printf("  Result:   %s\n", calculated_hex);

        if (strcmp(calculated_hex, test_cases[i].expected_hash) == 0) {

            printf("  Status:   \x1b[32mPASS\x1b[0m\n\n");
        } else {

            printf("  Status:   \x1b[31mFAIL\x1b[0m\n\n");
            failed_tests++;
        }
    }

    printf("--- Summary ---\n");
    if (failed_tests == 0) {
        printf("\x1b[32mAll 8 tests passed successfully!\x1b[0m\n");
    } else {
        printf("\x1b[31m%d out of 8 tests failed.\x1b[0m\n", failed_tests);
    }
}


int main() {
    // --- 1. Validity Verification ---
    printf("--- Correctness Test (Using C Wrapper with Diverse Inputs) ---\n");

    TestCase test_cases[8] = {
        {"", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"},
        {"abc", "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"},
        {"a", "ca978112ca1bbdcafac231b39a23dc4da786eff8147c4e72b9807785afee48bb"},
        {"hello world", "b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9"},
        {"1234567890", "c775e7b757ede630cd0aa1113bd102661ab38829ca52a6422ab782862f268646"},
        {"SHA256 AVX2 Test", "19f76accdedb73f6333c275e79cf840de7ec75f5e4ae18399f81830dd859ec36"},
        {"一生二，二生三，三生万物", "2c7b572cc2a7a3c0af7d2946f79973f15f3f56fd6fc6d85f2d2b5c8d272aa3ed"},
        {"https://github.com/8891689", "73f9f4d7581f581a8be499fd002160182f4953ab0a736494540161395fb94e81"}
    };

    Sha256Avx8_C_Handle* hasher = sha256_avx8_create();
    if (!hasher) {
        fprintf(stderr, "Failed to create hasher handle.\n");
        return 1;
    }

    alignas(64) uint8_t input_blocks[8][64];
    alignas(32) uint8_t final_hashes[8][32];

    for (int i = 0; i < 8; ++i) {
        prepare_test_data_block(input_blocks[i], test_cases[i].input, strlen(test_cases[i].input));
    }

    sha256_avx8_update_8_blocks(hasher, input_blocks);
    
    sha256_avx8_get_final_hashes(hasher, final_hashes);
    
    verify_and_print_results(test_cases, final_hashes);
    printf("\n");


    // --- 2. Performance Testing --- 
    printf("--- Performance Benchmark (Using C Wrapper) ---\n");
    const long long NUM_ITERATIONS = 5000000;
    const int BLOCKS_PER_CALL = 8;
    
    sha256_avx8_init(hasher); 

    for (int i = 0; i < 8; ++i) {
        prepare_test_data_block(input_blocks[i], "abc", 3);
    }

    for (int i = 0; i < 1000; ++i) {
        sha256_avx8_update_8_blocks(hasher, input_blocks);
    }
    
    sha256_avx8_init(hasher);

    clock_t start = clock();
    for (long long i = 0; i < NUM_ITERATIONS; ++i) {
        sha256_avx8_update_8_blocks(hasher, input_blocks);
    }
    clock_t end = clock();

    sha256_avx8_destroy(hasher);
    hasher = NULL;

    double total_time = (double)(end - start) / CLOCKS_PER_SEC;
    long long total_hashes = NUM_ITERATIONS * BLOCKS_PER_CALL;
    double hashes_per_sec = (double)total_hashes / total_time;
    double gigabytes_per_sec = (hashes_per_sec * 64) / (1024.0 * 1024.0 * 1024.0);

    printf("Total hashes processed: %lld\n", total_hashes);
    printf("Total time: %.4f seconds\n", total_time);
    printf("Performance: %.2f Million Hashes/sec\n", hashes_per_sec / 1e6);
    printf("Throughput:  %.2f GB/s\n", gigabytes_per_sec);

    return 0;
}
