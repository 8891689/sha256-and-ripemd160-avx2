// sha256_avx.h 
#ifndef SHA256_AVX_H
#define SHA256_AVX_H

#include <stdint.h>
#include <immintrin.h>

// --- SHA256_CTX_AVX8 和輔助宏 ---
typedef struct {
    __m256i state[8] __attribute__((aligned(64)));
} SHA256_CTX_AVX8;

// ROR_EPI32 實現
static inline __m256i vector_ror_epi32(__m256i x, int n) {
    return _mm256_or_si256(_mm256_srli_epi32(x, n), _mm256_slli_epi32(x, 32 - n));
}

// SHA256 的邏輯函數
#define CH_VEC(x, y, z) _mm256_xor_si256(_mm256_and_si256(x, y), _mm256_andnot_si256(x, z))
#define MAJ_VEC(x, y, z) _mm256_xor_si256(_mm256_xor_si256(_mm256_and_si256(x, y), _mm256_and_si256(x, z)), _mm256_and_si256(y, z))

// SHA256 的 Sigma 函數
#define STATE_SIGMA0_VEC_STD(x) _mm256_xor_si256(_mm256_xor_si256(vector_ror_epi32(x, 2), vector_ror_epi32(x, 13)), vector_ror_epi32(x, 22))
#define STATE_SIGMA1_VEC_STD(x) _mm256_xor_si256(_mm256_xor_si256(vector_ror_epi32(x, 6), vector_ror_epi32(x, 11)), vector_ror_epi32(x, 25))
#define MSG_SIGMA0_VEC_STD(x)   _mm256_xor_si256(_mm256_xor_si256(vector_ror_epi32(x, 7), vector_ror_epi32(x, 18)), _mm256_srli_epi32(x, 3))
#define MSG_SIGMA1_VEC_STD(x)   _mm256_xor_si256(_mm256_xor_si256(vector_ror_epi32(x, 17), vector_ror_epi32(x, 19)), _mm256_srli_epi32(x, 10))

// SHA-256 標準初始哈希值 H
#define SHA256_STD_H0 0x6a09e667
#define SHA256_STD_H1 0xbb67ae85
#define SHA256_STD_H2 0x3c6ef372
#define SHA256_STD_H3 0xa54ff53a
#define SHA256_STD_H4 0x510e527f
#define SHA256_STD_H5 0x9b05688c
#define SHA256_STD_H6 0x1f83d9ab
#define SHA256_STD_H7 0x5be0cd19

// 函數原型
void init_ctx_avx8_for_benchmark(SHA256_CTX_AVX8 *ctx);
void sha256_transform_avx8(SHA256_CTX_AVX8 *ctx, const uint8_t input_data_8blocks[8][64]);

#endif // SHA256_AVX_H
