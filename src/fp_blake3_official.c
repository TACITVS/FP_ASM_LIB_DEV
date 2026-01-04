/**
 * FP_BLAKE3_OFFICIAL - FP Wrapper around Official BLAKE3 Implementation
 *
 * Uses the official BLAKE3 team's highly optimized AVX2 implementation
 * wrapped with FP_ASM_LIB's purity guarantees.
 *
 * PURITY GUARANTEE:
 * - Input pointers are const and NEVER modified
 * - Output written to separate buffer
 * - No hidden global state (hasher is explicit parameter)
 * - Deterministic: same input -> same output
 */

#include "../vendor/blake3.h"
#include "fp_blake3.h"
#include <string.h>

/**
 * Hash using official BLAKE3 - PURE
 *
 * @param input  Input data (const - NEVER modified)
 * @param len    Input length
 * @param output Output buffer (32 bytes)
 */
void fp_blake3_hash_official(const uint8_t* input, size_t len, uint8_t* output) {
    blake3_hasher hasher;
    blake3_hasher_init(&hasher);
    blake3_hasher_update(&hasher, input, len);
    blake3_hasher_finalize(&hasher, output, BLAKE3_OUT_LEN);
}

/**
 * Keyed hash using official BLAKE3 - PURE
 */
void fp_blake3_hash_keyed_official(const uint8_t* key, const uint8_t* input,
                                    size_t len, uint8_t* output) {
    blake3_hasher hasher;
    blake3_hasher_init_keyed(&hasher, key);
    blake3_hasher_update(&hasher, input, len);
    blake3_hasher_finalize(&hasher, output, BLAKE3_OUT_LEN);
}

/**
 * Key derivation using official BLAKE3 - PURE
 */
void fp_blake3_derive_key_official(const char* context, size_t context_len,
                                    const uint8_t* key_material, size_t km_len,
                                    uint8_t* output) {
    blake3_hasher hasher;
    blake3_hasher_init_derive_key_raw(&hasher, context, context_len);
    blake3_hasher_update(&hasher, key_material, km_len);
    blake3_hasher_finalize(&hasher, output, BLAKE3_OUT_LEN);
}
