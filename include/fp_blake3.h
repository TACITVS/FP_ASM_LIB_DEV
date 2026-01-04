#pragma once

/**
 * FP_BLAKE3 - BLAKE3 Cryptographic Hash using FP_ASM_LIB primitives
 *
 * BLAKE3 is a cryptographic hash function that is:
 * - Fast: ~3x faster than SHA-256 on modern CPUs
 * - Secure: Based on ChaCha permutation, resistant to length extension
 * - Parallelizable: Tree structure enables multi-threaded hashing
 * - Versatile: Supports hashing, keyed hashing, and key derivation
 *
 * This implementation uses FP_ASM_LIB's AVX2-optimized u32 primitives
 * for the core mixing operations.
 *
 * Output: 256-bit (32-byte) hash
 * Block size: 64 bytes
 */

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Constants
 * ============================================================================ */

#define FP_BLAKE3_OUT_LEN       32    /* Default output length (256 bits) */
#define FP_BLAKE3_KEY_LEN       32    /* Key length for keyed hashing */
#define FP_BLAKE3_BLOCK_LEN     64    /* Block size in bytes */
#define FP_BLAKE3_CHUNK_LEN     1024  /* Chunk size for tree hashing */

/* ============================================================================
 * Types
 * ============================================================================ */

/**
 * BLAKE3 hasher state
 *
 * Maintains incremental hashing state for streaming input.
 */
typedef struct {
    uint32_t cv[8];           /* Chaining value (current state) */
    uint64_t chunk_counter;   /* Number of chunks processed */
    uint8_t  buf[FP_BLAKE3_BLOCK_LEN];  /* Partial block buffer */
    uint8_t  buf_len;         /* Bytes in buffer */
    uint8_t  blocks_compressed; /* Blocks compressed in current chunk */
    uint8_t  flags;           /* Domain separation flags */
    uint8_t  key[FP_BLAKE3_KEY_LEN];  /* Key (for keyed mode) */
} FpBlake3Hasher;

/**
 * BLAKE3 output structure
 */
typedef struct {
    uint8_t hash[FP_BLAKE3_OUT_LEN];
} FpBlake3Hash;

/* ============================================================================
 * Simple API (One-shot hashing)
 * ============================================================================ */

/**
 * Hash data in one call
 *
 * @param input  Input data to hash
 * @param len    Length of input in bytes
 * @param output Output buffer (32 bytes)
 */
void fp_blake3_hash(const uint8_t* input, size_t len, uint8_t* output);

/**
 * Hash data with key (MAC)
 *
 * @param key    32-byte key
 * @param input  Input data
 * @param len    Length of input
 * @param output Output buffer (32 bytes)
 */
void fp_blake3_hash_keyed(const uint8_t* key, const uint8_t* input,
                          size_t len, uint8_t* output);

/**
 * Derive a key from context and key material
 *
 * @param context      Context string (domain separator)
 * @param context_len  Length of context
 * @param key_material Input key material
 * @param km_len       Length of key material
 * @param output       Output buffer (32 bytes)
 */
void fp_blake3_derive_key(const char* context, size_t context_len,
                          const uint8_t* key_material, size_t km_len,
                          uint8_t* output);

/* ============================================================================
 * Incremental API (Streaming hashing)
 * ============================================================================ */

/**
 * Initialize hasher for incremental hashing
 *
 * @param hasher  Hasher state to initialize
 */
void fp_blake3_hasher_init(FpBlake3Hasher* hasher);

/**
 * Initialize hasher for keyed hashing
 *
 * @param hasher  Hasher state to initialize
 * @param key     32-byte key
 */
void fp_blake3_hasher_init_keyed(FpBlake3Hasher* hasher, const uint8_t* key);

/**
 * Add input data to hasher
 *
 * @param hasher  Hasher state
 * @param input   Input data
 * @param len     Length of input
 */
void fp_blake3_hasher_update(FpBlake3Hasher* hasher, const uint8_t* input, size_t len);

/**
 * Finalize hash and produce output
 *
 * @param hasher  Hasher state
 * @param output  Output buffer (32 bytes)
 */
void fp_blake3_hasher_finalize(const FpBlake3Hasher* hasher, uint8_t* output);

/**
 * Finalize with extended output (XOF mode)
 *
 * @param hasher      Hasher state
 * @param output      Output buffer
 * @param output_len  Desired output length (can be > 32 bytes)
 */
void fp_blake3_hasher_finalize_xof(const FpBlake3Hasher* hasher,
                                    uint8_t* output, size_t output_len);

/* ============================================================================
 * Low-level API (For advanced use)
 * ============================================================================ */

/**
 * BLAKE3 compression function (single block)
 *
 * This is the core primitive that transforms the state.
 * Uses AVX2-optimized operations from FP_ASM_LIB.
 *
 * @param state         8 x u32 state (modified in-place)
 * @param block         64-byte input block
 * @param block_len     Actual length (for padding)
 * @param counter       Block counter
 * @param flags         Domain flags
 */
void fp_blake3_compress(uint32_t state[8], const uint8_t block[64],
                        uint8_t block_len, uint64_t counter, uint8_t flags);

/**
 * BLAKE3 G mixing function (quarter-round)
 *
 * Core mixing operation, optimized with AVX2.
 * Operates on 4 words at positions a, b, c, d with messages mx, my.
 *
 * @param state  16 x u32 working state
 * @param a,b,c,d  Indices into state
 * @param mx, my   Message words
 */
void fp_blake3_g(uint32_t* state, int a, int b, int c, int d,
                 uint32_t mx, uint32_t my);

/**
 * BLAKE3 round function
 *
 * Applies 8 G functions in specific pattern.
 *
 * @param state  16 x u32 working state
 * @param msg    Message schedule (16 x u32)
 */
void fp_blake3_round(uint32_t* state, const uint32_t* msg);

/* ============================================================================
 * SIMD-Optimized Batch Operations
 * ============================================================================ */

/**
 * Hash multiple inputs in parallel (AVX2 optimized)
 *
 * Hashes up to 8 independent inputs simultaneously using SIMD.
 * Significant speedup for batch processing.
 *
 * @param inputs      Array of input pointers
 * @param lengths     Array of input lengths
 * @param outputs     Array of output buffers (32 bytes each)
 * @param count       Number of inputs (1-8)
 */
void fp_blake3_hash_batch(const uint8_t** inputs, const size_t* lengths,
                          uint8_t** outputs, size_t count);

/* ============================================================================
 * AVX2-Accelerated Functions
 * ============================================================================ */

/**
 * AVX2-accelerated hash (3-5x faster than scalar)
 */
void fp_blake3_hash_avx2(const uint8_t* input, size_t len, uint8_t* output);

/**
 * Maximum performance hash - all operations in registers
 */
void fp_blake3_hash_fast(const uint8_t* input, size_t len, uint8_t* output);

/**
 * Official BLAKE3 implementation (3-5x faster than SHA-256)
 */
void fp_blake3_hash_official(const uint8_t* input, size_t len, uint8_t* output);
void fp_blake3_hash_keyed_official(const uint8_t* key, const uint8_t* input,
                                    size_t len, uint8_t* output);
void fp_blake3_derive_key_official(const char* context, size_t context_len,
                                    const uint8_t* key_material, size_t km_len,
                                    uint8_t* output);

/**
 * AVX2-accelerated keyed hash
 */
void fp_blake3_hash_keyed_avx2(const uint8_t* key, const uint8_t* input,
                               size_t len, uint8_t* output);

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

/**
 * Convert hash to hexadecimal string
 *
 * @param hash   32-byte hash
 * @param hex    Output buffer (65 bytes: 64 hex chars + null)
 */
void fp_blake3_to_hex(const uint8_t* hash, char* hex);

/**
 * Compare two hashes in constant time
 *
 * @param a  First hash (32 bytes)
 * @param b  Second hash (32 bytes)
 * @return   0 if equal, non-zero otherwise
 */
int fp_blake3_compare(const uint8_t* a, const uint8_t* b);

#ifdef __cplusplus
}
#endif
