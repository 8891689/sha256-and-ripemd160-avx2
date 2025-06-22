// author： https://github.com/8891689
#include "sha256_avx.h"
#include <string.h> 
#include <stdio.h>  

// SHA-256 常量 K 的定义 (保持不变)
const uint32_t k_const[64] __attribute__((aligned(64))) = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

// 用於向量化字節序轉換 (bswap32) 的 shuffle mask 的數據
// 將 {B0,B1,B2,B3} 轉換為 {B3,B2,B1,B0}
const uint8_t bswap_mask_data[32] __attribute__((aligned(32))) = {
    // Lane 1 (高128位元) - 對應 _mm256_set_epi8 的前16個參數
    3, 2, 1, 0,  7, 6, 5, 4,  11,10, 9, 8,  15,14,13,12, // Word 7, Word 6, Word 5, Word 4
    // Lane 0 (低128位元) - 對應 _mm256_set_epi8 的後16個參數
    3, 2, 1, 0,  7, 6, 5, 4,  11,10, 9, 8,  15,14,13,12  // Word 3, Word 2, Word 1, Word 0
};

// 初始化 SHA256 上下文 (保持不变)
void init_ctx_avx8_for_benchmark(SHA256_CTX_AVX8 *ctx) {
    ctx->state[0] = _mm256_set1_epi32(SHA256_STD_H0);
    ctx->state[1] = _mm256_set1_epi32(SHA256_STD_H1);
    ctx->state[2] = _mm256_set1_epi32(SHA256_STD_H2);
    ctx->state[3] = _mm256_set1_epi32(SHA256_STD_H3);
    ctx->state[4] = _mm256_set1_epi32(SHA256_STD_H4);
    ctx->state[5] = _mm256_set1_epi32(SHA256_STD_H5);
    ctx->state[6] = _mm256_set1_epi32(SHA256_STD_H6);
    ctx->state[7] = _mm256_set1_epi32(SHA256_STD_H7);
}

// SHA256 核心轉換函數，支持 8 個数据塊並行處理
void sha256_transform_avx8(SHA256_CTX_AVX8 *ctx, const uint8_t input_data_8blocks[8][64]) {

    // 從全局數據加載 bswap_mask
    const __m256i bswap_mask = _mm256_load_si256((const __m256i*)bswap_mask_data);

    // --- 1. 消息調度（Message Schedule - W 的生成） ---
    uint32_t W_storage_raw[64][8] __attribute__((aligned(64)));
    __m256i* W_storage = (__m256i*)W_storage_raw;


    // --- Vectorized W[0..15] loading, transpose, and bswap ---
    __m256i B[8];

    for(int i=0; i<8; ++i) {
        B[i] = _mm256_loadu_si256((__m256i const*)(&input_data_8blocks[i][0]));
    }

    __m256i T0, T1, T2, T3, T4, T5, T6, T7;
    __m256i S0, S1, S2, S3, S4, S5, S6, S7;

    T0 = _mm256_unpacklo_epi32(B[0], B[1]); T1 = _mm256_unpackhi_epi32(B[0], B[1]);
    T2 = _mm256_unpacklo_epi32(B[2], B[3]); T3 = _mm256_unpackhi_epi32(B[2], B[3]);
    T4 = _mm256_unpacklo_epi32(B[4], B[5]); T5 = _mm256_unpackhi_epi32(B[4], B[5]);
    T6 = _mm256_unpacklo_epi32(B[6], B[7]); T7 = _mm256_unpackhi_epi32(B[6], B[7]);

    S0 = _mm256_unpacklo_epi64(T0, T2); S1 = _mm256_unpackhi_epi64(T0, T2);
    S2 = _mm256_unpacklo_epi64(T1, T3); S3 = _mm256_unpackhi_epi64(T1, T3);
    S4 = _mm256_unpacklo_epi64(T4, T6); S5 = _mm256_unpackhi_epi64(T4, T6);
    S6 = _mm256_unpacklo_epi64(T5, T7); S7 = _mm256_unpackhi_epi64(T5, T7);
    
    W_storage[0] = _mm256_shuffle_epi8(_mm256_permute2x128_si256(S0, S4, 0x20), bswap_mask);
    W_storage[1] = _mm256_shuffle_epi8(_mm256_permute2x128_si256(S0, S4, 0x31), bswap_mask);
    W_storage[2] = _mm256_shuffle_epi8(_mm256_permute2x128_si256(S1, S5, 0x20), bswap_mask);
    W_storage[3] = _mm256_shuffle_epi8(_mm256_permute2x128_si256(S1, S5, 0x31), bswap_mask);
    W_storage[4] = _mm256_shuffle_epi8(_mm256_permute2x128_si256(S2, S6, 0x20), bswap_mask);
    W_storage[5] = _mm256_shuffle_epi8(_mm256_permute2x128_si256(S2, S6, 0x31), bswap_mask);
    W_storage[6] = _mm256_shuffle_epi8(_mm256_permute2x128_si256(S3, S7, 0x20), bswap_mask);
    W_storage[7] = _mm256_shuffle_epi8(_mm256_permute2x128_si256(S3, S7, 0x31), bswap_mask);

    for(int i=0; i<8; ++i) {
        B[i] = _mm256_loadu_si256((__m256i const*)(&input_data_8blocks[i][32]));
    }

    T0 = _mm256_unpacklo_epi32(B[0], B[1]); T1 = _mm256_unpackhi_epi32(B[0], B[1]);
    T2 = _mm256_unpacklo_epi32(B[2], B[3]); T3 = _mm256_unpackhi_epi32(B[2], B[3]);
    T4 = _mm256_unpacklo_epi32(B[4], B[5]); T5 = _mm256_unpackhi_epi32(B[4], B[5]);
    T6 = _mm256_unpacklo_epi32(B[6], B[7]); T7 = _mm256_unpackhi_epi32(B[6], B[7]);

    S0 = _mm256_unpacklo_epi64(T0, T2); S1 = _mm256_unpackhi_epi64(T0, T2);
    S2 = _mm256_unpacklo_epi64(T1, T3); S3 = _mm256_unpackhi_epi64(T1, T3);
    S4 = _mm256_unpacklo_epi64(T4, T6); S5 = _mm256_unpackhi_epi64(T4, T6);
    S6 = _mm256_unpacklo_epi64(T5, T7); S7 = _mm256_unpackhi_epi64(T5, T7);

    W_storage[8]  = _mm256_shuffle_epi8(_mm256_permute2x128_si256(S0, S4, 0x20), bswap_mask);
    W_storage[9]  = _mm256_shuffle_epi8(_mm256_permute2x128_si256(S0, S4, 0x31), bswap_mask);
    W_storage[10] = _mm256_shuffle_epi8(_mm256_permute2x128_si256(S1, S5, 0x20), bswap_mask);
    W_storage[11] = _mm256_shuffle_epi8(_mm256_permute2x128_si256(S1, S5, 0x31), bswap_mask);
    W_storage[12] = _mm256_shuffle_epi8(_mm256_permute2x128_si256(S2, S6, 0x20), bswap_mask);
    W_storage[13] = _mm256_shuffle_epi8(_mm256_permute2x128_si256(S2, S6, 0x31), bswap_mask);
    W_storage[14] = _mm256_shuffle_epi8(_mm256_permute2x128_si256(S3, S7, 0x20), bswap_mask);
    W_storage[15] = _mm256_shuffle_epi8(_mm256_permute2x128_si256(S3, S7, 0x31), bswap_mask);


    // 生成 W[16..63]
    for (int i = 16; i < 64; ++i) {
        __m256i w_im2   = W_storage[i-2];
        __m256i w_im7   = W_storage[i-7];
        __m256i w_im15  = W_storage[i-15];
        __m256i w_im16  = W_storage[i-16];

        __m256i s1 = MSG_SIGMA1_VEC_STD(w_im2);
        __m256i s0 = MSG_SIGMA0_VEC_STD(w_im15);

        __m256i res = _mm256_add_epi32(s1, _mm256_add_epi32(w_im7, _mm256_add_epi32(w_im16, s0)));
        W_storage[i] = res;
    }

    // --- 2. SHA256 主循环 ---
    __m256i a = ctx->state[0];
    __m256i b = ctx->state[1];
    __m256i c = ctx->state[2];
    __m256i d = ctx->state[3];
    __m256i e = ctx->state[4];
    __m256i f = ctx->state[5];
    __m256i g = ctx->state[6];
    __m256i h = ctx->state[7];

    __m256i t1, t2;

    for (int i = 0; i < 64; i += 2) {
        __m256i k_vec_i = _mm256_set1_epi32(k_const[i]);
        __m256i W_vec_i = W_storage[i];

        t1 = _mm256_add_epi32(h, _mm256_add_epi32(STATE_SIGMA1_VEC_STD(e),
            _mm256_add_epi32(CH_VEC(e, f, g), _mm256_add_epi32(k_vec_i, W_vec_i))));
        t2 = _mm256_add_epi32(STATE_SIGMA0_VEC_STD(a), MAJ_VEC(a, b, c));

        h = g; g = f; f = e;
        e = _mm256_add_epi32(d, t1);
        d = c; c = b; b = a;
        a = _mm256_add_epi32(t1, t2);

        __m256i k_vec_i1 = _mm256_set1_epi32(k_const[i+1]);
        __m256i W_vec_i1 = W_storage[i+1];

        t1 = _mm256_add_epi32(h, _mm256_add_epi32(STATE_SIGMA1_VEC_STD(e),
            _mm256_add_epi32(CH_VEC(e, f, g), _mm256_add_epi32(k_vec_i1, W_vec_i1))));
        t2 = _mm256_add_epi32(STATE_SIGMA0_VEC_STD(a), MAJ_VEC(a, b, c));
        
        h = g; g = f; f = e;
        e = _mm256_add_epi32(d, t1);
        d = c; c = b; b = a;
        a = _mm256_add_epi32(t1, t2);
    }

    // 更新 ctx->state
    ctx->state[0] = _mm256_add_epi32(ctx->state[0], a);
    ctx->state[1] = _mm256_add_epi32(ctx->state[1], b);
    ctx->state[2] = _mm256_add_epi32(ctx->state[2], c);
    ctx->state[3] = _mm256_add_epi32(ctx->state[3], d);
    ctx->state[4] = _mm256_add_epi32(ctx->state[4], e);
    ctx->state[5] = _mm256_add_epi32(ctx->state[5], f);
    ctx->state[6] = _mm256_add_epi32(ctx->state[6], g);
    ctx->state[7] = _mm256_add_epi32(ctx->state[7], h);
}
