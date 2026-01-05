/**
 * FP_BLAKE3_PARALLEL - 4-Way Parallel BLAKE3 Compression
 *
 * Processes 4 independent compression operations simultaneously using AVX2.
 * This is where BLAKE3 gets its 3-5x speedup over SHA-256.
 *
 * Key insight: Within a chunk (1024 bytes = 16 blocks), we can process
 * blocks from 4 different chunks in parallel for multi-chunk inputs.
 * For single chunks, we use the scalar fast path.
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

/*
 * 4-WAY PARALLEL COMPRESSION
 *
 * Instead of: compress(cv, block) -> new_cv  (1 at a time)
 * We do:      compress4(cv[4], block[4]) -> new_cv[4]  (4 at a time!)
 *
 * State layout in YMM registers:
 *   ymm0 = [s0_a, s1_a, s2_a, s3_a, s0_b, s1_b, s2_b, s3_b]
 *          where s0..s3 are the 4 parallel states, a..b are state words
 *
 * Actually easier: transpose so each YMM holds same word from 4 states:
 *   ymm0 = [state0[0], state1[0], state2[0], state3[0], ...]
 */

/* Rotation helpers for 4-way parallel (YMM = 8 x u32) */
static inline __m256i rotr16_256(__m256i x) {
    return _mm256_shuffle_epi8(x, _mm256_setr_epi8(
        2,3,0,1, 6,7,4,5, 10,11,8,9, 14,15,12,13,
        2,3,0,1, 6,7,4,5, 10,11,8,9, 14,15,12,13));
}

static inline __m256i rotr8_256(__m256i x) {
    return _mm256_shuffle_epi8(x, _mm256_setr_epi8(
        1,2,3,0, 5,6,7,4, 9,10,11,8, 13,14,15,12,
        1,2,3,0, 5,6,7,4, 9,10,11,8, 13,14,15,12));
}

static inline __m256i rotr12_256(__m256i x) {
    return _mm256_or_si256(_mm256_srli_epi32(x, 12), _mm256_slli_epi32(x, 20));
}

static inline __m256i rotr7_256(__m256i x) {
    return _mm256_or_si256(_mm256_srli_epi32(x, 7), _mm256_slli_epi32(x, 25));
}

/**
 * 4-way parallel G function
 * Each YMM register holds the same position from 4 different states
 */
static inline void g4(
    __m256i* a, __m256i* b, __m256i* c, __m256i* d,
    __m256i mx, __m256i my
) {
    *a = _mm256_add_epi32(*a, *b);
    *a = _mm256_add_epi32(*a, mx);
    *d = _mm256_xor_si256(*d, *a);
    *d = rotr16_256(*d);

    *c = _mm256_add_epi32(*c, *d);
    *b = _mm256_xor_si256(*b, *c);
    *b = rotr12_256(*b);

    *a = _mm256_add_epi32(*a, *b);
    *a = _mm256_add_epi32(*a, my);
    *d = _mm256_xor_si256(*d, *a);
    *d = rotr8_256(*d);

    *c = _mm256_add_epi32(*c, *d);
    *b = _mm256_xor_si256(*b, *c);
    *b = rotr7_256(*b);
}

/**
 * 4-way parallel round
 */
static inline void round4(
    __m256i* v0, __m256i* v1, __m256i* v2, __m256i* v3,
    __m256i* v4, __m256i* v5, __m256i* v6, __m256i* v7,
    __m256i* v8, __m256i* v9, __m256i* v10, __m256i* v11,
    __m256i* v12, __m256i* v13, __m256i* v14, __m256i* v15,
    __m256i m0, __m256i m1, __m256i m2, __m256i m3,
    __m256i m4, __m256i m5, __m256i m6, __m256i m7,
    __m256i m8, __m256i m9, __m256i m10, __m256i m11,
    __m256i m12, __m256i m13, __m256i m14, __m256i m15
) {
    /* Column step */
    g4(v0, v4, v8, v12, m0, m1);
    g4(v1, v5, v9, v13, m2, m3);
    g4(v2, v6, v10, v14, m4, m5);
    g4(v3, v7, v11, v15, m6, m7);

    /* Diagonal step */
    g4(v0, v5, v10, v15, m8, m9);
    g4(v1, v6, v11, v12, m10, m11);
    g4(v2, v7, v8, v13, m12, m13);
    g4(v3, v4, v9, v14, m14, m15);
}

/**
 * Compress 4 blocks from 4 different chunks in parallel
 *
 * @param cv0-cv3: Chaining values for 4 chunks (each 8 x u32)
 * @param blocks: 4 x 64-byte blocks interleaved or sequential
 * @param counters: 4 chunk counters
 * @param block_lens: 4 block lengths
 * @param flags: 4 flag bytes
 * @param out0-out3: Output CVs
 */
static void compress4(
    const uint32_t cv0[8], const uint32_t cv1[8],


    const uint32_t cv2[8], const uint32_t cv3[8],
    const uint8_t* block0, const uint8_t* block1,
    const uint8_t* block2, const uint8_t* block3,
    uint64_t counter0, uint64_t counter1,
    uint64_t counter2, uint64_t counter3,
    uint8_t len0, uint8_t len1, uint8_t len2, uint8_t len3,
    uint8_t flags0, uint8_t flags1, uint8_t flags2, uint8_t flags3,
    uint32_t out0[8], uint32_t out1[8],
    uint32_t out2[8], uint32_t out3[8]
) {
    /* Load and transpose CVs into YMM registers */
    /* v[i] = [cv0[i], cv1[i], cv2[i], cv3[i], iv[i-8], ...] for i < 8 */

    __m256i v0 = _mm256_setr_epi32(cv0[0], cv1[0], cv2[0], cv3[0], cv0[0], cv1[0], cv2[0], cv3[0]);
    __m256i v1 = _mm256_setr_epi32(cv0[1], cv1[1], cv2[1], cv3[1], cv0[1], cv1[1], cv2[1], cv3[1]);
    __m256i v2 = _mm256_setr_epi32(cv0[2], cv1[2], cv2[2], cv3[2], cv0[2], cv1[2], cv2[2], cv3[2]);
    __m256i v3 = _mm256_setr_epi32(cv0[3], cv1[3], cv2[3], cv3[3], cv0[3], cv1[3], cv2[3], cv3[3]);
    __m256i v4 = _mm256_setr_epi32(cv0[4], cv1[4], cv2[4], cv3[4], cv0[4], cv1[4], cv2[4], cv3[4]);
    __m256i v5 = _mm256_setr_epi32(cv0[5], cv1[5], cv2[5], cv3[5], cv0[5], cv1[5], cv2[5], cv3[5]);
    __m256i v6 = _mm256_setr_epi32(cv0[6], cv1[6], cv2[6], cv3[6], cv0[6], cv1[6], cv2[6], cv3[6]);
    __m256i v7 = _mm256_setr_epi32(cv0[7], cv1[7], cv2[7], cv3[7], cv0[7], cv1[7], cv2[7], cv3[7]);

    /* v8-v11 = IV broadcast */
    __m256i v8 = _mm256_set1_epi32(IV[0]);
    __m256i v9 = _mm256_set1_epi32(IV[1]);
    __m256i v10 = _mm256_set1_epi32(IV[2]);
    __m256i v11 = _mm256_set1_epi32(IV[3]);

    /* v12-v15 = counters, lengths, flags */
    __m256i v12 = _mm256_setr_epi32(
        (uint32_t)counter0, (uint32_t)counter1, (uint32_t)counter2, (uint32_t)counter3,
        (uint32_t)counter0, (uint32_t)counter1, (uint32_t)counter2, (uint32_t)counter3);
    __m256i v13 = _mm256_setr_epi32(
        (uint32_t)(counter0>>32), (uint32_t)(counter1>>32), (uint32_t)(counter2>>32), (uint32_t)(counter3>>32),
        (uint32_t)(counter0>>32), (uint32_t)(counter1>>32), (uint32_t)(counter2>>32), (uint32_t)(counter3>>32));
    __m256i v14 = _mm256_setr_epi32(len0, len1, len2, len3, len0, len1, len2, len3);
    __m256i v15 = _mm256_setr_epi32(flags0, flags1, flags2, flags3, flags0, flags1, flags2, flags3);

    /* Load messages - need to gather from 4 blocks */
    uint32_t msg0[16], msg1[16], msg2[16], msg3[16];
    for (int i = 0; i < 16; i++) {
        msg0[i] = *(uint32_t*)(block0 + i*4);
        msg1[i] = *(uint32_t*)(block1 + i*4);
        msg2[i] = *(uint32_t*)(block2 + i*4);
        msg3[i] = *(uint32_t*)(block3 + i*4);
    }

    /* 7 rounds */
    for (int round = 0; round < 7; round++) {
        const uint8_t* s = MSG_SCHEDULE[round];

        /* Load permuted messages */
        __m256i m0 = _mm256_setr_epi32(msg0[s[0]], msg1[s[0]], msg2[s[0]], msg3[s[0]],
                                        msg0[s[0]], msg1[s[0]], msg2[s[0]], msg3[s[0]]);
        __m256i m1 = _mm256_setr_epi32(msg0[s[1]], msg1[s[1]], msg2[s[1]], msg3[s[1]],
                                        msg0[s[1]], msg1[s[1]], msg2[s[1]], msg3[s[1]]);
        __m256i m2 = _mm256_setr_epi32(msg0[s[2]], msg1[s[2]], msg2[s[2]], msg3[s[2]],
                                        msg0[s[2]], msg1[s[2]], msg2[s[2]], msg3[s[2]]);
        __m256i m3 = _mm256_setr_epi32(msg0[s[3]], msg1[s[3]], msg2[s[3]], msg3[s[3]],
                                        msg0[s[3]], msg1[s[3]], msg2[s[3]], msg3[s[3]]);
        __m256i m4 = _mm256_setr_epi32(msg0[s[4]], msg1[s[4]], msg2[s[4]], msg3[s[4]],
                                        msg0[s[4]], msg1[s[4]], msg2[s[4]], msg3[s[4]]);
        __m256i m5 = _mm256_setr_epi32(msg0[s[5]], msg1[s[5]], msg2[s[5]], msg3[s[5]],
                                        msg0[s[5]], msg1[s[5]], msg2[s[5]], msg3[s[5]]);
        __m256i m6 = _mm256_setr_epi32(msg0[s[6]], msg1[s[6]], msg2[s[6]], msg3[s[6]],
                                        msg0[s[6]], msg1[s[6]], msg2[s[6]], msg3[s[6]]);
        __m256i m7 = _mm256_setr_epi32(msg0[s[7]], msg1[s[7]], msg2[s[7]], msg3[s[7]],
                                        msg0[s[7]], msg1[s[7]], msg2[s[7]], msg3[s[7]]);
        __m256i m8 = _mm256_setr_epi32(msg0[s[8]], msg1[s[8]], msg2[s[8]], msg3[s[8]],
                                        msg0[s[8]], msg1[s[8]], msg2[s[8]], msg3[s[8]]);
        __m256i m9 = _mm256_setr_epi32(msg0[s[9]], msg1[s[9]], msg2[s[9]], msg3[s[9]],
                                        msg0[s[9]], msg1[s[9]], msg2[s[9]], msg3[s[9]]);
        __m256i m10 = _mm256_setr_epi32(msg0[s[10]], msg1[s[10]], msg2[s[10]], msg3[s[10]],
                                         msg0[s[10]], msg1[s[10]], msg2[s[10]], msg3[s[10]]);
        __m256i m11 = _mm256_setr_epi32(msg0[s[11]], msg1[s[11]], msg2[s[11]], msg3[s[11]],
                                         msg0[s[11]], msg1[s[11]], msg2[s[11]], msg3[s[11]]);
        __m256i m12 = _mm256_setr_epi32(msg0[s[12]], msg1[s[12]], msg2[s[12]], msg3[s[12]],
                                         msg0[s[12]], msg1[s[12]], msg2[s[12]], msg3[s[12]]);
        __m256i m13 = _mm256_setr_epi32(msg0[s[13]], msg1[s[13]], msg2[s[13]], msg3[s[13]],
                                         msg0[s[13]], msg1[s[13]], msg2[s[13]], msg3[s[13]]);
        __m256i m14 = _mm256_setr_epi32(msg0[s[14]], msg1[s[14]], msg2[s[14]], msg3[s[14]],
                                         msg0[s[14]], msg1[s[14]], msg2[s[14]], msg3[s[14]]);
        __m256i m15 = _mm256_setr_epi32(msg0[s[15]], msg1[s[15]], msg2[s[15]], msg3[s[15]],
                                         msg0[s[15]], msg1[s[15]], msg2[s[15]], msg3[s[15]]);

        round4(&v0, &v1, &v2, &v3, &v4, &v5, &v6, &v7,
               &v8, &v9, &v10, &v11, &v12, &v13, &v14, &v15,
               m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15);
    }

    /* Finalize: XOR v[i] ^= v[i+8] */
    v0 = _mm256_xor_si256(v0, v8);
    v1 = _mm256_xor_si256(v1, v9);
    v2 = _mm256_xor_si256(v2, v10);
    v3 = _mm256_xor_si256(v3, v11);
    v4 = _mm256_xor_si256(v4, v12);
    v5 = _mm256_xor_si256(v5, v13);
    v6 = _mm256_xor_si256(v6, v14);
    v7 = _mm256_xor_si256(v7, v15);

    /* Extract results - need to transpose back */
    /* v0 = [out0[0], out1[0], out2[0], out3[0], ...] */
    uint32_t tmp[8];

    _mm256_storeu_si256((__m256i*)tmp, v0);
    out0[0] = tmp[0]; out1[0] = tmp[1]; out2[0] = tmp[2]; out3[0] = tmp[3];

    _mm256_storeu_si256((__m256i*)tmp, v1);
    out0[1] = tmp[0]; out1[1] = tmp[1]; out2[1] = tmp[2]; out3[1] = tmp[3];

    _mm256_storeu_si256((__m256i*)tmp, v2);
    out0[2] = tmp[0]; out1[2] = tmp[1]; out2[2] = tmp[2]; out3[2] = tmp[3];

    _mm256_storeu_si256((__m256i*)tmp, v3);
    out0[3] = tmp[0]; out1[3] = tmp[1]; out2[3] = tmp[2]; out3[3] = tmp[3];

    _mm256_storeu_si256((__m256i*)tmp, v4);
    out0[4] = tmp[0]; out1[4] = tmp[1]; out2[4] = tmp[2]; out3[4] = tmp[3];

    _mm256_storeu_si256((__m256i*)tmp, v5);
    out0[5] = tmp[0]; out1[5] = tmp[1]; out2[5] = tmp[2]; out3[5] = tmp[3];

    _mm256_storeu_si256((__m256i*)tmp, v6);
    out0[6] = tmp[0]; out1[6] = tmp[1]; out2[6] = tmp[2]; out3[6] = tmp[3];

    _mm256_storeu_si256((__m256i*)tmp, v7);
    out0[7] = tmp[0]; out1[7] = tmp[1]; out2[7] = tmp[2]; out3[7] = tmp[3];
}

static void write_cv(const uint32_t cv[8], uint8_t* output) {
    for (int i = 0; i < 8; i++) {
        output[i*4] = (uint8_t)cv[i];
        output[i*4+1] = (uint8_t)(cv[i] >> 8);
        output[i*4+2] = (uint8_t)(cv[i] >> 16);
        output[i*4+3] = (uint8_t)(cv[i] >> 24);
    }
}

static void parent_cv(const uint32_t left[8], const uint32_t right[8],
                      uint8_t flags, uint32_t out[8]) {
    uint8_t block[FP_BLAKE3_BLOCK_LEN];
    for (int i = 0; i < 8; i++) {
        block[i*4] = (uint8_t)left[i];
        block[i*4+1] = (uint8_t)(left[i] >> 8);
        block[i*4+2] = (uint8_t)(left[i] >> 16);
        block[i*4+3] = (uint8_t)(left[i] >> 24);
    }
    for (int i = 0; i < 8; i++) {
        block[32+i*4] = (uint8_t)right[i];
        block[32+i*4+1] = (uint8_t)(right[i] >> 8);
        block[32+i*4+2] = (uint8_t)(right[i] >> 16);
        block[32+i*4+3] = (uint8_t)(right[i] >> 24);
    }

    uint32_t cv[8];
    memcpy(cv, IV, sizeof(IV));
    fp_blake3_compress(cv, block, FP_BLAKE3_BLOCK_LEN, 0, flags);
    memcpy(out, cv, sizeof(cv));
}

static void chunk_to_cv_local(const uint8_t* input, size_t len,
                              uint64_t counter, uint8_t flags,
                              uint32_t cv_out[8]) {
    uint32_t cv[8];
    memcpy(cv, IV, sizeof(IV));

    size_t offset = 0;
    int blocks = 0;

    while (len - offset > FP_BLAKE3_BLOCK_LEN) {
        uint8_t block_flags = flags;
        if (blocks == 0) {
            block_flags |= CHUNK_START;
        }

        fp_blake3_compress(cv, input + offset, FP_BLAKE3_BLOCK_LEN, counter, block_flags);
        offset += FP_BLAKE3_BLOCK_LEN;
        blocks++;
    }

    uint8_t last[FP_BLAKE3_BLOCK_LEN] = {0};
    size_t remaining = len - offset;
    memcpy(last, input + offset, remaining);

    uint8_t block_flags = flags | CHUNK_END;
    if (blocks == 0) {
        block_flags |= CHUNK_START;
    }

    fp_blake3_compress(cv, last, (uint8_t)remaining, counter, block_flags);
    memcpy(cv_out, cv, sizeof(cv));
}

/**
 * Hash large input using 4-way parallel compression
 */
void fp_blake3_hash_parallel(const uint8_t* input, size_t len, uint8_t* output)
{
    if (len <= FP_BLAKE3_CHUNK_LEN) {
        /* Small input: use fast scalar path */
        fp_blake3_hash_fast(input, len, output);
        return;
    }

    uint32_t stack[54][8];
    int stack_depth = 0;
    uint64_t chunk_counter = 0;
    const uint8_t* ptr = input;
    size_t remaining = len;

    /* Process 4 chunks at a time when possible */
    while (remaining >= 4 * FP_BLAKE3_CHUNK_LEN) {
        uint32_t cv0[8], cv1[8], cv2[8], cv3[8];
        memcpy(cv0, IV, 32); memcpy(cv1, IV, 32);
        memcpy(cv2, IV, 32); memcpy(cv3, IV, 32);

        /* Process each chunk's 16 blocks */
        for (int b = 0; b < 16; b++) {
            uint8_t flags = (b == 0) ? CHUNK_START : 0;
            if (b == 15) flags |= CHUNK_END;

            uint32_t out0[8], out1[8], out2[8], out3[8];
            compress4(
                cv0, cv1, cv2, cv3,
                ptr + b*FP_BLAKE3_BLOCK_LEN,
                ptr + FP_BLAKE3_CHUNK_LEN + b*FP_BLAKE3_BLOCK_LEN,
                ptr + 2*FP_BLAKE3_CHUNK_LEN + b*FP_BLAKE3_BLOCK_LEN,
                ptr + 3*FP_BLAKE3_CHUNK_LEN + b*FP_BLAKE3_BLOCK_LEN,
                chunk_counter, chunk_counter + 1, chunk_counter + 2, chunk_counter + 3,
                FP_BLAKE3_BLOCK_LEN, FP_BLAKE3_BLOCK_LEN, FP_BLAKE3_BLOCK_LEN, FP_BLAKE3_BLOCK_LEN,
                flags, flags, flags, flags,
                out0, out1, out2, out3
            );
            memcpy(cv0, out0, 32); memcpy(cv1, out1, 32);
            memcpy(cv2, out2, 32); memcpy(cv3, out3, 32);
        }

        const uint32_t* cvs[4] = {cv0, cv1, cv2, cv3};
        for (int i = 0; i < 4; i++) {
            memcpy(stack[stack_depth], cvs[i], sizeof(cv0));
            stack_depth++;

            uint64_t total = chunk_counter + (uint64_t)i + 1;
            while (stack_depth >= 2 && (total & 1) == 0) {
                uint32_t merged[8];
                parent_cv(stack[stack_depth - 2], stack[stack_depth - 1], PARENT, merged);
                stack_depth--;
                memcpy(stack[stack_depth - 1], merged, sizeof(merged));
                total >>= 1;
            }
        }

        chunk_counter += 4;
        ptr += 4 * FP_BLAKE3_CHUNK_LEN;
        remaining -= 4 * FP_BLAKE3_CHUNK_LEN;
    }

    while (remaining > FP_BLAKE3_CHUNK_LEN) {
        uint32_t chunk_cv[8];
        chunk_to_cv_local(ptr, FP_BLAKE3_CHUNK_LEN, chunk_counter, 0, chunk_cv);

        memcpy(stack[stack_depth], chunk_cv, sizeof(chunk_cv));
        stack_depth++;

        uint64_t total = chunk_counter + 1;
        while (stack_depth >= 2 && (total & 1) == 0) {
            uint32_t merged[8];
            parent_cv(stack[stack_depth - 2], stack[stack_depth - 1], PARENT, merged);
            stack_depth--;
            memcpy(stack[stack_depth - 1], merged, sizeof(merged));
            total >>= 1;
        }

        ptr += FP_BLAKE3_CHUNK_LEN;
        remaining -= FP_BLAKE3_CHUNK_LEN;
        chunk_counter++;
    }

    if (remaining == 0) {
        if (stack_depth == 0) {
            fp_blake3_hash_fast(input, len, output);
            return;
        }

        while (stack_depth > 1) {
            uint8_t merge_flags = (stack_depth == 2) ? (PARENT | ROOT) : PARENT;
            uint32_t merged[8];
            parent_cv(stack[stack_depth - 2], stack[stack_depth - 1], merge_flags, merged);
            stack_depth--;
            memcpy(stack[stack_depth - 1], merged, sizeof(merged));
        }

        write_cv(stack[0], output);
        return;
    }

    uint32_t final_cv[8];
    uint8_t final_flags = (stack_depth == 0) ? ROOT : 0;
    chunk_to_cv_local(ptr, remaining, chunk_counter, final_flags, final_cv);

    if (stack_depth == 0) {
        write_cv(final_cv, output);
        return;
    }

    memcpy(stack[stack_depth], final_cv, sizeof(final_cv));
    stack_depth++;

    while (stack_depth > 1) {
        uint8_t merge_flags = (stack_depth == 2) ? (PARENT | ROOT) : PARENT;
        uint32_t merged[8];
        parent_cv(stack[stack_depth - 2], stack[stack_depth - 1], merge_flags, merged);
        stack_depth--;
        memcpy(stack[stack_depth - 1], merged, sizeof(merged));
    }

    write_cv(stack[0], output);
}
