#ifndef FP_BITMAP_H
#define FP_BITMAP_H

#include <stdint.h>
#include <stddef.h>

// Set a bit in a bitmap (array of uint64_t)
// bitmap: pointer to bitmap array
// index: bit index to set
void fp_bitmap_set(uint64_t* bitmap, size_t index);

// Bitwise AND of two bitmaps
// a: first bitmap (input)
// b: second bitmap (input)
// result: output bitmap (must be pre-allocated)
// n_u64: number of uint64_t elements in each bitmap
void fp_bitmap_and(const uint64_t* a, const uint64_t* b, uint64_t* result, size_t n_u64);

#endif // FP_BITMAP_H
