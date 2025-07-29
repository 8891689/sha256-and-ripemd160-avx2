/*
* main_full_avx.c
*
* Implements fully chained parallel HASH160 using custom AVX2 SHA-256 and AVX2 RIPEMD-160.
* Implements pure AVX2 processing of multi-block inputs (e.g., 65-byte uncompressed public keys) by manually splitting the data into chunks.
*
* Compilation instructions:
* gcc -O3 -mavx2 -march=native main_full_avx.c sha256_avx.c ripemd160_avx.c -o main_full_test -lsecp256k1 -lcrypto
*/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <stdalign.h>
#include <stdbool.h>

#include "sha256_avx.h"
#include "ripemd160_avx.h"

#include <secp256k1.h>
#include <openssl/sha.h>
#include <openssl/ripemd.h> // Only used for final verification

// --- Auxiliary functions ---

void print_hex(const char* label, const unsigned char* data, size_t len) {
    printf("%s", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02x", data[i]);
    }
    printf("\n");
}

void increment_privkey(unsigned char key[32]) {
    for (int i = 31; i >= 0; i--) {
        if (key[i] < 0xFF) {
            key[i]++;
            return;
        }
        key[i] = 0x00;
    }
}

// Auxiliary functions for verification 
void calculate_single_hash160_openssl(const unsigned char* input, size_t input_len, unsigned char output[20]) {
    unsigned char sha256_digest[32];
    SHA256(input, input_len, sha256_digest);
    RIPEMD160(sha256_digest, 32, output);
}

/**
* @brief Prepares data for a SHA256 hash that requires two blocks.
* @param block1 [output] Buffer to store the first 64-byte block.
* @param block2 [output] Buffer to store the second 64-byte block.
* @param input_data [input] Raw input data (e.g., a 65-byte public key).
* @param input_len [input] Length of the raw input data.
*/
void prepare_multi_block_sha256_data(uint8_t block1[64], uint8_t block2[64], const uint8_t* input_data, size_t input_len) {
    // Assert that the input length is greater than one block and less than two blocks
    assert(input_len > 64 && input_len < 128);

    // Prepare the first block: directly copy the first 64 bytes
    memcpy(block1, input_data, 64);

    // Prepare the second block
    memset(block2, 0, 64);
    size_t remaining_len = input_len - 64;
    memcpy(block2, input_data + 64, remaining_len); // Copy the remaining data
    block2[remaining_len] = 0x80; // Add padding

    // Add length information (big endian)
    uint64_t total_bits = input_len * 8;
    for (int i = 0; i < 8; i++) {
        block2[63 - i] = (total_bits >> (i * 8)) & 0xFF;
    }
}


int main(int argc, char **argv) {
    long long total_pubkeys = 100000;
    if (argc > 1) {
        total_pubkeys = atoll(argv[1]);
        if (total_pubkeys <= 0) total_pubkeys = 100000;
    }

    const int BATCH_SIZE = LANE_COUNT;
    if (total_pubkeys % BATCH_SIZE != 0) {
        total_pubkeys = (total_pubkeys / BATCH_SIZE + 1) * BATCH_SIZE;
        printf("Adjusting total public keys to be a multiple of %d: %lld\n", BATCH_SIZE, total_pubkeys);
    }
    const long long NUM_BATCHES = total_pubkeys / BATCH_SIZE;

    secp256k1_context* secp_ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
    Sha256Avx8_C_Handle* sha_hasher = sha256_avx8_create();
    RIPEMD160_MULTI_CTX ripemd_ctx;

    assert(secp_ctx != NULL && sha_hasher != NULL);

    unsigned char privkey[32] = {0};
    privkey[31] = 1;

    // Data block (single block) prepared for compressed public key
    alignas(64) uint8_t comp_pubkey_blocks[BATCH_SIZE][64];
    
    // Data blocks for uncompressed public keys (two blocks)
    alignas(64) uint8_t uncomp_pubkey_blocks_1[BATCH_SIZE][64];
    alignas(64) uint8_t uncomp_pubkey_blocks_2[BATCH_SIZE][64];
    
    // Intermediate and final result storage
    alignas(32) uint8_t sha256_results[BATCH_SIZE][32];
    alignas(32) uint8_t ripemd_results_comp[BATCH_SIZE][20];
    alignas(32) uint8_t ripemd_results_uncomp[BATCH_SIZE][20];

    printf("Starting %lld HASH160 calculations using PURE AVX2 pipeline...\n", total_pubkeys);
    printf("  - Both key types handled by multi-block AVX2-SHA256 -> AVX2-RIPEMD160.\n");
    printf("Processing in batches of %d. Results for the last 5 public keys will be printed.\n\n", BATCH_SIZE);

    clock_t start = clock();

    for (long long batch_idx = 0; batch_idx < NUM_BATCHES; batch_idx++) {
        
        unsigned char last_batch_privkeys[BATCH_SIZE][32];
        unsigned char last_batch_comp_keys[BATCH_SIZE][33];
        unsigned char last_batch_uncomp_keys[BATCH_SIZE][65];
        size_t last_batch_comp_len[BATCH_SIZE];
        size_t last_batch_uncomp_len[BATCH_SIZE];

        // 1. Generate a batch of public keys and prepare data blocks
        for (int i = 0; i < BATCH_SIZE; i++) {
            bool is_last_batch = (batch_idx == NUM_BATCHES - 1);
            if (is_last_batch) {
                memcpy(last_batch_privkeys[i], privkey, 32);
            }

            secp256k1_pubkey pubkey_obj;
            assert(secp256k1_ec_seckey_verify(secp_ctx, privkey) == 1);
            assert(secp256k1_ec_pubkey_create(secp_ctx, &pubkey_obj, privkey) == 1);
            
            size_t uncomp_len = 65, comp_len = 33;
            unsigned char uncomp_buf[65], comp_buf[33];
            secp256k1_ec_pubkey_serialize(secp_ctx, uncomp_buf, &uncomp_len, &pubkey_obj, SECP256K1_EC_UNCOMPRESSED);
            secp256k1_ec_pubkey_serialize(secp_ctx, comp_buf, &comp_len, &pubkey_obj, SECP256K1_EC_COMPRESSED);
            
            // Prepare single block data for compressed public key
            prepare_test_data_block(comp_pubkey_blocks[i], (const char*)comp_buf, comp_len);
            
            // Prepare dual-block data for uncompressed public keys
            prepare_multi_block_sha256_data(uncomp_pubkey_blocks_1[i], uncomp_pubkey_blocks_2[i], uncomp_buf, uncomp_len);

            if (is_last_batch) {
                 memcpy(last_batch_comp_keys[i], comp_buf, comp_len);
                 last_batch_comp_len[i] = comp_len;
                 memcpy(last_batch_uncomp_keys[i], uncomp_buf, uncomp_len);
                 last_batch_uncomp_len[i] = uncomp_len;
            }
            increment_privkey(privkey);
        }

        // --- 2. Handle compressed format public keys (single-block AVX link) ---
        sha256_avx8_init(sha_hasher);
        sha256_avx8_update_8_blocks(sha_hasher, comp_pubkey_blocks);
        sha256_avx8_get_final_hashes(sha_hasher, sha256_results);
        
        ripemd160_multi_init(&ripemd_ctx);
        for(int i = 0; i < BATCH_SIZE; i++) {
            memcpy(ripemd_ctx.buffer[i], sha256_results[i], 32);
            ripemd_ctx.buffer_len[i] = 32;
        }
        ripemd160_multi_final(&ripemd_ctx, ripemd_results_comp);

        // --- 3. Handle uncompressed public keys (dual-block AVX links) ---
        sha256_avx8_init(sha_hasher);
        sha256_avx8_update_8_blocks(sha_hasher, uncomp_pubkey_blocks_1); // Processing the first block
        sha256_avx8_update_8_blocks(sha_hasher, uncomp_pubkey_blocks_2); // Processing the second block
        sha256_avx8_get_final_hashes(sha_hasher, sha256_results);
        
        ripemd160_multi_init(&ripemd_ctx);
        for(int i = 0; i < BATCH_SIZE; i++) {
            memcpy(ripemd_ctx.buffer[i], sha256_results[i], 32);
            ripemd_ctx.buffer_len[i] = 32;
        }
        ripemd160_multi_final(&ripemd_ctx, ripemd_results_uncomp);
        
        // --- 4. Verify the results of the last batch ---
        if (batch_idx == NUM_BATCHES - 1) {
            printf("\n--- Results from the final batch (Verification) ---\n");
            for (int i = BATCH_SIZE - 5; i < BATCH_SIZE; ++i) {
                 long long current_pubkey_index = (batch_idx * BATCH_SIZE) + i + 1;
                printf("--- Public Key Index: %lld ---\n", current_pubkey_index);
                print_hex("  Private Key:             ", last_batch_privkeys[i], 32);
                
                unsigned char ref_comp_hash[20], ref_uncomp_hash[20];
                calculate_single_hash160_openssl(last_batch_comp_keys[i], last_batch_comp_len[i], ref_comp_hash);
                calculate_single_hash160_openssl(last_batch_uncomp_keys[i], last_batch_uncomp_len[i], ref_uncomp_hash);
                
                print_hex("  HASH160 (Comp, Pure AVX):", ripemd_results_comp[i], 20);
                print_hex("  HASH160 (Comp, OpenSSL): ", ref_comp_hash, 20);
                if(memcmp(ripemd_results_comp[i], ref_comp_hash, 20) == 0) printf("  (Comp Verified OK)\n");
                else printf("  (!!! Comp Verification FAILED !!!)\n");
                
                print_hex("  HASH160 (Uncomp, Pure AVX):", ripemd_results_uncomp[i], 20);
                print_hex("  HASH160 (Uncomp, OpenSSL):", ref_uncomp_hash, 20);
                if(memcmp(ripemd_results_uncomp[i], ref_uncomp_hash, 20) == 0) printf("  (Uncomp Verified OK)\n");
                else printf("  (!!! Uncomp Verification FAILED !!!)\n");
                printf("\n");
            }
        }
    }

    clock_t end = clock();

    secp256k1_context_destroy(secp_ctx);
    sha256_avx8_destroy(sha_hasher);

    double total_time = (double)(end - start) / CLOCKS_PER_SEC;
    long long total_hashes = total_pubkeys * 2;
    double hashes_per_sec = (double)total_hashes / total_time;

    printf("--- Performance Summary (Pure AVX2 Pipeline) ---\n");
    printf("Total public keys processed: %lld\n", total_pubkeys);
    printf("Total HASH160s computed:     %lld (2 per pubkey)\n", total_hashes);
    printf("Total time:                  %.4f seconds\n", total_time);
    printf("Performance:                 %.2f HASH160s/sec\n", hashes_per_sec);
    printf("Performance:                 %.2f Million HASH160s/sec\n", hashes_per_sec / 1e6);

    return 0;
}
