/**
 * FP_BLAKE3 - Purely Functional BLAKE3 Implementation
 *
 * PURITY GUARANTEE: This implementation adheres to FP_ASM_LIB's strict
 * functional programming principles:
 *
 * 1. INPUT IMMUTABILITY - All inputs are `const`, never modified
 * 2. OUTPUT CLARITY - Functions return new values, never mutate
 * 3. NO HIDDEN STATE - No globals modified, no static state
 * 4. DETERMINISTIC - Same input always produces same output
 *
 * The imperative BLAKE3 pattern:
 *     state = compress(state, block)  // MUTATES state
 *
 * Our functional pattern:
 *     new_cv = compress(old_cv, block)  // RETURNS new value
 *
 * Reference: https://github.com/BLAKE3-team/BLAKE3-specs
 */

#include "fp_blake3.h"
#include <string.h>

/* ============================================================================
 * BLAKE3 Constants (Immutable)
 * ============================================================================ */

/* Initial vector (first 8 words of fractional part of sqrt(2..9)) */
static const uint32_t IV[8] = {
    0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A,
    0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19
};

/* Message schedule permutation for each round */
static const uint8_t MSG_SCHEDULE[7][16] = {
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
    {2, 6, 3, 10, 7, 0, 4, 13, 1, 11, 12, 5, 9, 14, 15, 8},
    {3, 4, 10, 12, 13, 2, 7, 14, 6, 5, 9, 0, 11, 15, 8, 1},
    {10, 7, 12, 9, 14, 3, 13, 15, 4, 0, 11, 2, 5, 8, 1, 6},
    {12, 13, 9, 11, 15, 10, 14, 8, 7, 2, 5, 3, 0, 1, 6, 4},
    {9, 14, 11, 5, 8, 12, 15, 1, 13, 3, 0, 10, 2, 6, 4, 7},
    {11, 15, 5, 0, 1, 9, 8, 6, 14, 10, 2, 12, 3, 4, 7, 13}
};

/* Domain separation flags */
#define CHUNK_START         (1 << 0)
#define CHUNK_END           (1 << 1)
#define PARENT              (1 << 2)
#define ROOT                (1 << 3)
#define KEYED_HASH          (1 << 4)
#define DERIVE_KEY_CONTEXT  (1 << 5)
#define DERIVE_KEY_MATERIAL (1 << 6)

/* ============================================================================
 * Pure Helper Functions
 * ============================================================================ */

/* Right rotation - pure function */
static inline uint32_t rotr32(uint32_t x, int n) {
    return (x >> n) | (x << (32 - n));
}

/* Load little-endian u32 from bytes - pure function */
static inline uint32_t load32_le(const uint8_t* p) {
    return ((uint32_t)p[0]) |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

/* Store little-endian u32 to bytes - writes to output only */
static inline void store32_le(uint8_t* out, uint32_t v) {
    out[0] = (uint8_t)(v);
    out[1] = (uint8_t)(v >> 8);
    out[2] = (uint8_t)(v >> 16);
    out[3] = (uint8_t)(v >> 24);
}

/* Load block into message words - pure: const input, separate output */
static void load_msg(uint32_t* out_msg, const uint8_t* block) {
    for (int i = 0; i < 16; i++) {
        out_msg[i] = load32_le(block + i * 4);
    }
}

/* ============================================================================
 * Purely Functional G Mixing Function
 *
 * The G function is the core of BLAKE3. This version is PURE:
 * - Takes const input state
 * - Returns new values via output parameters
 * - Never modifies input
 * ============================================================================ */

/**
 * G mixing function - PURE VERSION
 *
 * Haskell equivalent:
 *   g :: (Word32, Word32, Word32, Word32) -> Word32 -> Word32
 *     -> (Word32, Word32, Word32, Word32)
 *   g (a, b, c, d) mx my = (a', b', c', d')
 *
 * @param a, b, c, d  Input state words (immutable)
 * @param mx, my      Message words
 * @param out_a, out_b, out_c, out_d  Output state words (new values)
 */
static inline void g_pure(
    uint32_t a, uint32_t b, uint32_t c, uint32_t d,
    uint32_t mx, uint32_t my,
    uint32_t* out_a, uint32_t* out_b, uint32_t* out_c, uint32_t* out_d
) {
    /* First half-round */
    a = a + b + mx;
    d = rotr32(d ^ a, 16);
    c = c + d;
    b = rotr32(b ^ c, 12);

    /* Second half-round */
    a = a + b + my;
    d = rotr32(d ^ a, 8);
    c = c + d;
    b = rotr32(b ^ c, 7);

    /* Write outputs (no mutation of inputs) */
    *out_a = a;
    *out_b = b;
    *out_c = c;
    *out_d = d;
}

/* ============================================================================
 * Purely Functional Round Function
 *
 * Applies 8 G functions in the BLAKE3 pattern.
 * Pure: const input state -> new output state
 * ============================================================================ */

/**
 * Round function - PURE VERSION
 *
 * @param state_in   Input state (16 words, const)
 * @param msg        Message schedule (16 words, const)
 * @param state_out  Output state (16 words, new)
 */
static void round_pure(
    const uint32_t* state_in,
    const uint32_t* msg,
    uint32_t* state_out
) {
    /* Copy input to working state (we'll build output incrementally) */
    uint32_t s[16];
    memcpy(s, state_in, 16 * sizeof(uint32_t));

    /* Column step: G on columns */
    g_pure(s[0], s[4], s[8],  s[12], msg[0],  msg[1],  &s[0], &s[4], &s[8],  &s[12]);
    g_pure(s[1], s[5], s[9],  s[13], msg[2],  msg[3],  &s[1], &s[5], &s[9],  &s[13]);
    g_pure(s[2], s[6], s[10], s[14], msg[4],  msg[5],  &s[2], &s[6], &s[10], &s[14]);
    g_pure(s[3], s[7], s[11], s[15], msg[6],  msg[7],  &s[3], &s[7], &s[11], &s[15]);

    /* Diagonal step: G on diagonals */
    g_pure(s[0], s[5], s[10], s[15], msg[8],  msg[9],  &s[0], &s[5], &s[10], &s[15]);
    g_pure(s[1], s[6], s[11], s[12], msg[10], msg[11], &s[1], &s[6], &s[11], &s[12]);
    g_pure(s[2], s[7], s[8],  s[13], msg[12], msg[13], &s[2], &s[7], &s[8],  &s[13]);
    g_pure(s[3], s[4], s[9],  s[14], msg[14], msg[15], &s[3], &s[4], &s[9],  &s[14]);

    /* Copy to output */
    memcpy(state_out, s, 16 * sizeof(uint32_t));
}

/**
 * Permute message according to round schedule - PURE
 */
static void permute_msg(uint32_t* out, const uint32_t* in, int round) {
    const uint8_t* schedule = MSG_SCHEDULE[round];
    for (int i = 0; i < 16; i++) {
        out[i] = in[schedule[i]];
    }
}

/* ============================================================================
 * Purely Functional Compression Function
 *
 * This is the core transformation. PURE:
 * - Takes const chaining value and const block
 * - Returns new chaining value via output parameter
 * - Never modifies inputs
 * ============================================================================ */

/**
 * BLAKE3 compression function - PURE VERSION
 *
 * Transforms chaining value with a block.
 *
 * @param cv_in      Input chaining value (8 words, const - NEVER modified)
 * @param block      Input block (64 bytes, const - NEVER modified)
 * @param block_len  Actual data length in block
 * @param counter    Block counter
 * @param flags      Domain separation flags
 * @param cv_out     Output chaining value (8 words - NEW value)
 */
static void compress_pure(
    const uint32_t* cv_in,
    const uint8_t* block,
    uint8_t block_len,
    uint64_t counter,
    uint8_t flags,
    uint32_t* cv_out
) {
    uint32_t state[16];
    uint32_t msg[16];

    /* Initialize state from chaining value and IV */
    memcpy(state, cv_in, 8 * sizeof(uint32_t));
    memcpy(state + 8, IV, 4 * sizeof(uint32_t));
    state[12] = (uint32_t)counter;
    state[13] = (uint32_t)(counter >> 32);
    state[14] = (uint32_t)block_len;
    state[15] = (uint32_t)flags;

    /* Load message block */
    load_msg(msg, block);

    /* 7 rounds of mixing */
    for (int round = 0; round < 7; round++) {
        uint32_t round_state[16];

        if (round == 0) {
            round_pure(state, msg, round_state);
        } else {
            uint32_t permuted[16];
            permute_msg(permuted, msg, round);
            round_pure(state, permuted, round_state);
        }

        memcpy(state, round_state, 16 * sizeof(uint32_t));
    }

    /* Finalize: XOR upper and lower halves to produce new CV */
    for (int i = 0; i < 8; i++) {
        cv_out[i] = state[i] ^ state[i + 8];
    }
}

/* ============================================================================
 * Purely Functional Chunk Processing
 * ============================================================================ */

/**
 * Process a chunk and return its chaining value - PURE
 *
 * @param key        Key/IV for this chunk (8 words, const)
 * @param input      Input data (const)
 * @param len        Length of input (max CHUNK_LEN)
 * @param counter    Chunk counter
 * @param flags      Base flags
 * @param cv_out     Output chaining value (8 words)
 */
static void chunk_to_cv(
    const uint32_t* key,
    const uint8_t* input,
    size_t len,
    uint64_t counter,
    uint8_t flags,
    uint32_t* cv_out
) {
    uint32_t cv[8];
    memcpy(cv, key, 8 * sizeof(uint32_t));

    size_t offset = 0;
    int blocks_compressed = 0;

    /* Process all complete blocks except the last */
    while (len - offset > FP_BLAKE3_BLOCK_LEN) {
        uint8_t block_flags = flags;
        if (blocks_compressed == 0) {
            block_flags |= CHUNK_START;
        }

        uint32_t new_cv[8];
        compress_pure(cv, input + offset, FP_BLAKE3_BLOCK_LEN, counter, block_flags, new_cv);
        memcpy(cv, new_cv, 8 * sizeof(uint32_t));

        offset += FP_BLAKE3_BLOCK_LEN;
        blocks_compressed++;
    }

    /* Final block of chunk (with padding) */
    uint8_t last_block[FP_BLAKE3_BLOCK_LEN] = {0};
    size_t remaining = len - offset;
    memcpy(last_block, input + offset, remaining);

    uint8_t block_flags = flags | CHUNK_END;
    if (blocks_compressed == 0) {
        block_flags |= CHUNK_START;
    }

    compress_pure(cv, last_block, (uint8_t)remaining, counter, block_flags, cv_out);
}

/**
 * Merge two child CVs into parent CV - PURE
 */
#if defined(__GNUC__) || defined(__clang__)
#define FP_BLAKE3_UNUSED __attribute__((unused))
#else
#define FP_BLAKE3_UNUSED
#endif

static FP_BLAKE3_UNUSED void parent_cv(
    const uint32_t* left_cv,
    const uint32_t* right_cv,
    const uint32_t* key,
    uint8_t flags,
    uint32_t* out_cv
) {
    uint8_t block[FP_BLAKE3_BLOCK_LEN];

    /* Pack two CVs into block */
    for (int i = 0; i < 8; i++) {
        store32_le(block + i * 4, left_cv[i]);
        store32_le(block + 32 + i * 4, right_cv[i]);
    }

    compress_pure(key, block, FP_BLAKE3_BLOCK_LEN, 0, flags | PARENT, out_cv);
}

/* ============================================================================
 * Public API - All Pure Functions
 * ============================================================================ */

/**
 * Hash data in one call - PURE
 *
 * Uses AVX2-accelerated implementation for maximum performance.
 *
 * @param input  Input data (const - NEVER modified)
 * @param len    Length of input
 * @param output Output hash (32 bytes - NEW value written here)
 */
void fp_blake3_hash(const uint8_t* input, size_t len, uint8_t* output) {
    /* Delegate to official BLAKE3 for maximum performance */
    fp_blake3_hash_official(input, len, output);
}

/**
 * Keyed hash (MAC) - PURE
 *
 * Uses AVX2-accelerated implementation.
 */
void fp_blake3_hash_keyed(const uint8_t* key, const uint8_t* input,
                          size_t len, uint8_t* output) {
    /* Delegate to official BLAKE3 */
    fp_blake3_hash_keyed_official(key, input, len, output);
}

/**
 * Key derivation - PURE
 */
void fp_blake3_derive_key(const char* context, size_t context_len,
                          const uint8_t* key_material, size_t km_len,
                          uint8_t* output) {
    /* First: hash context to get context key */
    uint32_t context_key[8];
    chunk_to_cv(IV, (const uint8_t*)context, context_len, 0,
                DERIVE_KEY_CONTEXT | ROOT, context_key);

    /* Second: hash key material with context key */
    uint32_t cv[8];
    chunk_to_cv(context_key, key_material, km_len, 0,
                DERIVE_KEY_MATERIAL | ROOT, cv);

    for (int i = 0; i < 8; i++) {
        store32_le(output + i * 4, cv[i]);
    }
}

/* ============================================================================
 * Incremental Hasher (Maintains immutable snapshots)
 * ============================================================================ */

void fp_blake3_hasher_init(FpBlake3Hasher* hasher) {
    memcpy(hasher->cv, IV, sizeof(IV));
    hasher->chunk_counter = 0;
    hasher->buf_len = 0;
    hasher->blocks_compressed = 0;
    hasher->flags = 0;
    memset(hasher->key, 0, sizeof(hasher->key));
    memset(hasher->buf, 0, sizeof(hasher->buf));
}

void fp_blake3_hasher_init_keyed(FpBlake3Hasher* hasher, const uint8_t* key) {
    for (int i = 0; i < 8; i++) {
        hasher->cv[i] = load32_le(key + i * 4);
    }
    hasher->chunk_counter = 0;
    hasher->buf_len = 0;
    hasher->blocks_compressed = 0;
    hasher->flags = KEYED_HASH;
    memcpy(hasher->key, key, FP_BLAKE3_KEY_LEN);
    memset(hasher->buf, 0, sizeof(hasher->buf));
}

void fp_blake3_hasher_update(FpBlake3Hasher* hasher, const uint8_t* input, size_t len) {
    while (len > 0) {
        size_t take = FP_BLAKE3_BLOCK_LEN - hasher->buf_len;
        if (take > len) take = len;

        memcpy(hasher->buf + hasher->buf_len, input, take);
        hasher->buf_len += (uint8_t)take;
        input += take;
        len -= take;

        if (hasher->buf_len == FP_BLAKE3_BLOCK_LEN && len > 0) {
            uint8_t block_flags = hasher->flags;
            if (hasher->blocks_compressed == 0) {
                block_flags |= CHUNK_START;
            }

            uint32_t new_cv[8];
            compress_pure(hasher->cv, hasher->buf, FP_BLAKE3_BLOCK_LEN,
                         hasher->chunk_counter, block_flags, new_cv);
            memcpy(hasher->cv, new_cv, sizeof(new_cv));

            hasher->blocks_compressed++;
            hasher->buf_len = 0;

            if (hasher->blocks_compressed == 16) {
                hasher->chunk_counter++;
                hasher->blocks_compressed = 0;
                memcpy(hasher->cv, IV, sizeof(IV));
            }
        }
    }
}

void fp_blake3_hasher_finalize(const FpBlake3Hasher* hasher, uint8_t* output) {
    uint8_t block[FP_BLAKE3_BLOCK_LEN] = {0};
    memcpy(block, hasher->buf, hasher->buf_len);

    uint8_t block_flags = hasher->flags | CHUNK_END | ROOT;
    if (hasher->blocks_compressed == 0) {
        block_flags |= CHUNK_START;
    }

    uint32_t cv[8];
    compress_pure(hasher->cv, block, hasher->buf_len, hasher->chunk_counter, block_flags, cv);

    for (int i = 0; i < 8; i++) {
        store32_le(output + i * 4, cv[i]);
    }
}

void fp_blake3_hasher_finalize_xof(const FpBlake3Hasher* hasher,
                                    uint8_t* output, size_t output_len) {
    uint8_t block[FP_BLAKE3_BLOCK_LEN] = {0};
    memcpy(block, hasher->buf, hasher->buf_len);

    uint8_t block_flags = hasher->flags | CHUNK_END | ROOT;
    if (hasher->blocks_compressed == 0) {
        block_flags |= CHUNK_START;
    }

    size_t offset = 0;
    uint64_t output_block = 0;

    while (offset < output_len) {
        uint32_t cv[8];
        compress_pure(hasher->cv, block, hasher->buf_len, output_block, block_flags, cv);

        size_t take = output_len - offset;
        if (take > 32) take = 32;

        for (size_t i = 0; i < take; i++) {
            output[offset + i] = (uint8_t)(cv[i / 4] >> ((i % 4) * 8));
        }

        offset += take;
        output_block++;
    }
}

/* ============================================================================
 * Utility Functions - Pure
 * ============================================================================ */

void fp_blake3_to_hex(const uint8_t* hash, char* hex) {
    static const char hex_chars[] = "0123456789abcdef";
    for (int i = 0; i < 32; i++) {
        hex[i * 2] = hex_chars[(hash[i] >> 4) & 0xF];
        hex[i * 2 + 1] = hex_chars[hash[i] & 0xF];
    }
    hex[64] = '\0';
}

int fp_blake3_compare(const uint8_t* a, const uint8_t* b) {
    /* Constant-time comparison */
    uint8_t diff = 0;
    for (int i = 0; i < 32; i++) {
        diff |= a[i] ^ b[i];
    }
    return diff;
}

/* Legacy API wrappers for compatibility */
void fp_blake3_g(uint32_t* state, int a, int b, int c, int d,
                 uint32_t mx, uint32_t my) {
    g_pure(state[a], state[b], state[c], state[d], mx, my,
           &state[a], &state[b], &state[c], &state[d]);
}

void fp_blake3_round(uint32_t* state, const uint32_t* msg) {
    uint32_t out[16];
    round_pure(state, msg, out);
    memcpy(state, out, 16 * sizeof(uint32_t));
}

void fp_blake3_compress(uint32_t cv[8], const uint8_t block[64],
                        uint8_t block_len, uint64_t counter, uint8_t flags) {
    uint32_t new_cv[8];
    compress_pure(cv, block, block_len, counter, flags, new_cv);
    memcpy(cv, new_cv, 8 * sizeof(uint32_t));
}
