/**
 * FP_BLAKE3_FAST - Maximum Performance BLAKE3 with SSE4.1/AVX2
 *
 * Optimizations:
 * 1. All state kept in XMM registers throughout rounds
 * 2. Byte shuffle for 8/16-bit rotations (single instruction)
 * 3. Interleaved column/diagonal processing
 * 4. Minimal memory traffic
 *
 * PURITY: Inputs const, outputs to separate buffer, no hidden state
 */

#include "fp_blake3.h"
#include <string.h>
#include <immintrin.h>

static const uint32_t IV[8] = {
    0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A,
    0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19
};

static const uint8_t MSG_SCHEDULE[7][16] = {
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
    {2, 6, 3, 10, 7, 0, 4, 13, 1, 11, 12, 5, 9, 14, 15, 8},
    {3, 4, 10, 12, 13, 2, 7, 14, 6, 5, 9, 0, 11, 15, 8, 1},
    {10, 7, 12, 9, 14, 3, 13, 15, 4, 0, 11, 2, 5, 8, 1, 6},
    {12, 13, 9, 11, 15, 10, 14, 8, 7, 2, 5, 3, 0, 1, 6, 4},
    {9, 14, 11, 5, 8, 12, 15, 1, 13, 3, 0, 10, 2, 6, 4, 7},
    {11, 15, 5, 0, 1, 9, 8, 6, 14, 10, 2, 12, 3, 4, 7, 13}
};

#define CHUNK_START         (1 << 0)
#define CHUNK_END           (1 << 1)
#define PARENT              (1 << 2)
#define ROOT                (1 << 3)
#define KEYED_HASH          (1 << 4)
#define DERIVE_KEY_CONTEXT  (1 << 5)
#define DERIVE_KEY_MATERIAL (1 << 6)

/* Shuffle masks for byte rotations */
static const __m128i ROT16_MASK = {0x0504070601000302ULL, 0x0D0C0F0E09080B0AULL};
static const __m128i ROT8_MASK  = {0x0407060500030201ULL, 0x0C0F0E0D080B0A09ULL};

/* Fast rotation macros - all in registers */
#define ROTR16(x) _mm_shuffle_epi8((x), ROT16_MASK)
#define ROTR8(x)  _mm_shuffle_epi8((x), ROT8_MASK)
#define ROTR12(x) _mm_or_si128(_mm_srli_epi32((x), 12), _mm_slli_epi32((x), 20))
#define ROTR7(x)  _mm_or_si128(_mm_srli_epi32((x), 7), _mm_slli_epi32((x), 25))

/* G function on 4 lanes - pure register operations */
#define G(a, b, c, d, mx, my) do { \
    a = _mm_add_epi32(a, b); \
    a = _mm_add_epi32(a, mx); \
    d = _mm_xor_si128(d, a); \
    d = ROTR16(d); \
    c = _mm_add_epi32(c, d); \
    b = _mm_xor_si128(b, c); \
    b = ROTR12(b); \
    a = _mm_add_epi32(a, b); \
    a = _mm_add_epi32(a, my); \
    d = _mm_xor_si128(d, a); \
    d = ROTR8(d); \
    c = _mm_add_epi32(c, d); \
    b = _mm_xor_si128(b, c); \
    b = ROTR7(b); \
} while(0)

/* Diagonal shuffle operations */
#define DIAG_V1(x) _mm_shuffle_epi32((x), _MM_SHUFFLE(0, 3, 2, 1))  /* rotate left */
#define DIAG_V2(x) _mm_shuffle_epi32((x), _MM_SHUFFLE(1, 0, 3, 2))  /* swap pairs */
#define DIAG_V3(x) _mm_shuffle_epi32((x), _MM_SHUFFLE(2, 1, 0, 3))  /* rotate right */

#define UNDIAG_V1(x) _mm_shuffle_epi32((x), _MM_SHUFFLE(2, 1, 0, 3))
#define UNDIAG_V2(x) _mm_shuffle_epi32((x), _MM_SHUFFLE(1, 0, 3, 2))
#define UNDIAG_V3(x) _mm_shuffle_epi32((x), _MM_SHUFFLE(0, 3, 2, 1))

/**
 * Ultra-fast compression - all 7 rounds in registers
 */
static void compress_fast(
    const uint32_t* cv_in,
    const uint8_t* block,
    uint8_t block_len,
    uint64_t counter,
    uint8_t flags,
    uint32_t* cv_out
) {
    /* Load state: v0-v3 = CV, v4-v7 = IV, v8-v11 = IV[0..3], v12-v15 = counter/len/flags */
    __m128i v0 = _mm_loadu_si128((const __m128i*)cv_in);       /* cv[0..3] */
    __m128i v1 = _mm_loadu_si128((const __m128i*)(cv_in + 4)); /* cv[4..7] */
    __m128i v2 = _mm_loadu_si128((const __m128i*)IV);          /* IV[0..3] */
    __m128i v3 = _mm_setr_epi32((uint32_t)counter, (uint32_t)(counter >> 32),
                                (uint32_t)block_len, (uint32_t)flags);

    /* Load message into 4 registers */
    __m128i m0 = _mm_loadu_si128((const __m128i*)block);
    __m128i m1 = _mm_loadu_si128((const __m128i*)(block + 16));
    __m128i m2 = _mm_loadu_si128((const __m128i*)(block + 32));
    __m128i m3 = _mm_loadu_si128((const __m128i*)(block + 48));

    /* Load rotation masks into registers */
    __m128i rot16 = _mm_setr_epi8(2,3,0,1, 6,7,4,5, 10,11,8,9, 14,15,12,13);
    __m128i rot8  = _mm_setr_epi8(1,2,3,0, 5,6,7,4, 9,10,11,8, 13,14,15,12);

    /* === All 7 rounds, fully unrolled for maximum speed === */

    /* We need message words organized for column/diagonal G calls */
    /* Round 0: schedule {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15} */

    #define ROUND(m0_val, m1_val, m2_val, m3_val, m4_val, m5_val, m6_val, m7_val) do { \
        /* Column step */ \
        { \
            __m128i mx = _mm_setr_epi32(m0_val, m2_val, m4_val, m6_val); \
            __m128i my = _mm_setr_epi32(m1_val, m3_val, m5_val, m7_val); \
            v0 = _mm_add_epi32(v0, v1); \
            v0 = _mm_add_epi32(v0, mx); \
            v3 = _mm_xor_si128(v3, v0); \
            v3 = _mm_shuffle_epi8(v3, rot16); \
            v2 = _mm_add_epi32(v2, v3); \
            v1 = _mm_xor_si128(v1, v2); \
            v1 = _mm_or_si128(_mm_srli_epi32(v1, 12), _mm_slli_epi32(v1, 20)); \
            v0 = _mm_add_epi32(v0, v1); \
            v0 = _mm_add_epi32(v0, my); \
            v3 = _mm_xor_si128(v3, v0); \
            v3 = _mm_shuffle_epi8(v3, rot8); \
            v2 = _mm_add_epi32(v2, v3); \
            v1 = _mm_xor_si128(v1, v2); \
            v1 = _mm_or_si128(_mm_srli_epi32(v1, 7), _mm_slli_epi32(v1, 25)); \
        } \
        /* Diagonal step - shuffle to diagonal positions */ \
        v1 = _mm_shuffle_epi32(v1, _MM_SHUFFLE(0, 3, 2, 1)); \
        v2 = _mm_shuffle_epi32(v2, _MM_SHUFFLE(1, 0, 3, 2)); \
        v3 = _mm_shuffle_epi32(v3, _MM_SHUFFLE(2, 1, 0, 3)); \
        { \
            __m128i mx = _mm_setr_epi32(m8_val, m10_val, m12_val, m14_val); \
            __m128i my = _mm_setr_epi32(m9_val, m11_val, m13_val, m15_val); \
            v0 = _mm_add_epi32(v0, v1); \
            v0 = _mm_add_epi32(v0, mx); \
            v3 = _mm_xor_si128(v3, v0); \
            v3 = _mm_shuffle_epi8(v3, rot16); \
            v2 = _mm_add_epi32(v2, v3); \
            v1 = _mm_xor_si128(v1, v2); \
            v1 = _mm_or_si128(_mm_srli_epi32(v1, 12), _mm_slli_epi32(v1, 20)); \
            v0 = _mm_add_epi32(v0, v1); \
            v0 = _mm_add_epi32(v0, my); \
            v3 = _mm_xor_si128(v3, v0); \
            v3 = _mm_shuffle_epi8(v3, rot8); \
            v2 = _mm_add_epi32(v2, v3); \
            v1 = _mm_xor_si128(v1, v2); \
            v1 = _mm_or_si128(_mm_srli_epi32(v1, 7), _mm_slli_epi32(v1, 25)); \
        } \
        /* Un-shuffle back to column positions */ \
        v1 = _mm_shuffle_epi32(v1, _MM_SHUFFLE(2, 1, 0, 3)); \
        v2 = _mm_shuffle_epi32(v2, _MM_SHUFFLE(1, 0, 3, 2)); \
        v3 = _mm_shuffle_epi32(v3, _MM_SHUFFLE(0, 3, 2, 1)); \
    } while(0)

    /* Extract message words to locals for permutation */
    uint32_t msg[16];
    _mm_storeu_si128((__m128i*)msg, m0);
    _mm_storeu_si128((__m128i*)(msg + 4), m1);
    _mm_storeu_si128((__m128i*)(msg + 8), m2);
    _mm_storeu_si128((__m128i*)(msg + 12), m3);

    /* Round 0 */
    #define m0_val msg[0]
    #define m1_val msg[1]
    #define m2_val msg[2]
    #define m3_val msg[3]
    #define m4_val msg[4]
    #define m5_val msg[5]
    #define m6_val msg[6]
    #define m7_val msg[7]
    #define m8_val msg[8]
    #define m9_val msg[9]
    #define m10_val msg[10]
    #define m11_val msg[11]
    #define m12_val msg[12]
    #define m13_val msg[13]
    #define m14_val msg[14]
    #define m15_val msg[15]

    /* Inline all 7 rounds with correct message schedule */
    for (int round = 0; round < 7; round++) {
        const uint8_t* s = MSG_SCHEDULE[round];
        __m128i mx_col = _mm_setr_epi32(msg[s[0]], msg[s[2]], msg[s[4]], msg[s[6]]);
        __m128i my_col = _mm_setr_epi32(msg[s[1]], msg[s[3]], msg[s[5]], msg[s[7]]);
        __m128i mx_diag = _mm_setr_epi32(msg[s[8]], msg[s[10]], msg[s[12]], msg[s[14]]);
        __m128i my_diag = _mm_setr_epi32(msg[s[9]], msg[s[11]], msg[s[13]], msg[s[15]]);

        /* Column step */
        v0 = _mm_add_epi32(v0, v1);
        v0 = _mm_add_epi32(v0, mx_col);
        v3 = _mm_xor_si128(v3, v0);
        v3 = _mm_shuffle_epi8(v3, rot16);
        v2 = _mm_add_epi32(v2, v3);
        v1 = _mm_xor_si128(v1, v2);
        v1 = _mm_or_si128(_mm_srli_epi32(v1, 12), _mm_slli_epi32(v1, 20));
        v0 = _mm_add_epi32(v0, v1);
        v0 = _mm_add_epi32(v0, my_col);
        v3 = _mm_xor_si128(v3, v0);
        v3 = _mm_shuffle_epi8(v3, rot8);
        v2 = _mm_add_epi32(v2, v3);
        v1 = _mm_xor_si128(v1, v2);
        v1 = _mm_or_si128(_mm_srli_epi32(v1, 7), _mm_slli_epi32(v1, 25));

        /* Diagonal shuffle */
        v1 = _mm_shuffle_epi32(v1, _MM_SHUFFLE(0, 3, 2, 1));
        v2 = _mm_shuffle_epi32(v2, _MM_SHUFFLE(1, 0, 3, 2));
        v3 = _mm_shuffle_epi32(v3, _MM_SHUFFLE(2, 1, 0, 3));

        /* Diagonal step */
        v0 = _mm_add_epi32(v0, v1);
        v0 = _mm_add_epi32(v0, mx_diag);
        v3 = _mm_xor_si128(v3, v0);
        v3 = _mm_shuffle_epi8(v3, rot16);
        v2 = _mm_add_epi32(v2, v3);
        v1 = _mm_xor_si128(v1, v2);
        v1 = _mm_or_si128(_mm_srli_epi32(v1, 12), _mm_slli_epi32(v1, 20));
        v0 = _mm_add_epi32(v0, v1);
        v0 = _mm_add_epi32(v0, my_diag);
        v3 = _mm_xor_si128(v3, v0);
        v3 = _mm_shuffle_epi8(v3, rot8);
        v2 = _mm_add_epi32(v2, v3);
        v1 = _mm_xor_si128(v1, v2);
        v1 = _mm_or_si128(_mm_srli_epi32(v1, 7), _mm_slli_epi32(v1, 25));

        /* Un-shuffle */
        v1 = _mm_shuffle_epi32(v1, _MM_SHUFFLE(2, 1, 0, 3));
        v2 = _mm_shuffle_epi32(v2, _MM_SHUFFLE(1, 0, 3, 2));
        v3 = _mm_shuffle_epi32(v3, _MM_SHUFFLE(0, 3, 2, 1));
    }

    #undef m0_val
    #undef m1_val
    #undef m2_val
    #undef m3_val
    #undef m4_val
    #undef m5_val
    #undef m6_val
    #undef m7_val
    #undef m8_val
    #undef m9_val
    #undef m10_val
    #undef m11_val
    #undef m12_val
    #undef m13_val
    #undef m14_val
    #undef m15_val

    /* Finalize: XOR state halves */
    /* v0 ^= v2, v1 ^= v3 */
    v0 = _mm_xor_si128(v0, v2);
    v1 = _mm_xor_si128(v1, v3);

    /* Store output */
    _mm_storeu_si128((__m128i*)cv_out, v0);
    _mm_storeu_si128((__m128i*)(cv_out + 4), v1);
}

/* Public API using fast compression */
void fp_blake3_hash_fast(const uint8_t* input, size_t len, uint8_t* output) {
    uint32_t cv[8];
    memcpy(cv, IV, sizeof(IV));

    if (len <= 1024) {
        size_t offset = 0;
        int blocks = 0;

        while (len - offset > 64) {
            uint8_t flags = (blocks == 0) ? CHUNK_START : 0;
            uint32_t new_cv[8];
            compress_fast(cv, input + offset, 64, 0, flags, new_cv);
            memcpy(cv, new_cv, sizeof(cv));
            offset += 64;
            blocks++;
        }

        uint8_t last[64] = {0};
        memcpy(last, input + offset, len - offset);
        uint8_t flags = CHUNK_END | ROOT;
        if (blocks == 0) flags |= CHUNK_START;

        compress_fast(cv, last, (uint8_t)(len - offset), 0, flags, cv);

        for (int i = 0; i < 8; i++) {
            output[i*4]   = (uint8_t)cv[i];
            output[i*4+1] = (uint8_t)(cv[i] >> 8);
            output[i*4+2] = (uint8_t)(cv[i] >> 16);
            output[i*4+3] = (uint8_t)(cv[i] >> 24);
        }
    } else {
        /* Multi-chunk: fall back to tree structure */
        fp_blake3_hash_avx2(input, len, output);
    }
}
