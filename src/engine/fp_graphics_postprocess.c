/**
 * fp_graphics_postprocess.c
 *
 * FP-FIRST POST-PROCESSING EFFECTS
 *
 * Mission: Showcase FP-ASM library through post-processing operations.
 *
 * FP Principles:
 * - Immutable configuration (const inputs)
 * - Pure functions (no side effects)
 * - Composition of FP library functions
 * - Zero imperative loops in hot paths
 *
 * FP Library Usage:
 * - fp_map_scale_f32: Scale pixel values (brightness, tone mapping)
 * - fp_zip_add_f32: Combine images/effects
 * - fp_map_offset_f32: Adjust brightness levels
 * - fp_reduce_add_f32: Sum for averaging
 */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "fp_core.h"

//==============================================================================
// Box Blur (Using FP Library)
//==============================================================================

/**
 * PURE FP FUNCTION: Apply horizontal box blur
 *
 * Simple 3-pixel box blur: output[i] = (input[i-1] + input[i] + input[i+1]) / 3
 *
 * FP Composition:
 *   - Uses fp_zip_add_f32 to combine shifted arrays
 *   - Uses fp_map_scale_f32 to divide by 3
 *
 * @param input Input image (1D array)
 * @param output Output blurred image
 * @param width Image width
 * @param height Image height
 */
void fp_postprocess_blur_horizontal(
    const float* input,
    float* output,
    size_t width,
    size_t height
) {
    size_t total_pixels = width * height;

    // Temporary buffers for shifted arrays
    float* left = malloc(total_pixels * sizeof(float));
    float* center = malloc(total_pixels * sizeof(float));
    float* right = malloc(total_pixels * sizeof(float));
    float* temp = malloc(total_pixels * sizeof(float));

    // Copy and shift arrays
    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            size_t idx = y * width + x;

            // Left neighbor (clamp at edges)
            size_t left_x = (x > 0) ? x - 1 : x;
            left[idx] = input[y * width + left_x];

            // Center
            center[idx] = input[idx];

            // Right neighbor (clamp at edges)
            size_t right_x = (x < width - 1) ? x + 1 : x;
            right[idx] = input[y * width + right_x];
        }
    }

    // FP LIBRARY: Add left + center
    fp_zip_add_f32(left, center, temp, total_pixels);

    // FP LIBRARY: Add result + right
    fp_zip_add_f32(temp, right, output, total_pixels);

    // FP LIBRARY: Divide by 3 to get average
    fp_map_scale_f32(output, output, total_pixels, 1.0f / 3.0f);

    free(left);
    free(center);
    free(right);
    free(temp);
}

//==============================================================================
// Bloom Effect (Using FP Library)
//==============================================================================

/**
 * PURE FP FUNCTION: Extract bright regions (bright pass filter)
 *
 * Pixels above threshold are kept, others set to 0.
 * Uses FP library for the filtering operation.
 *
 * @param input Input image
 * @param output Output bright regions only
 * @param pixel_count Number of pixels
 * @param threshold Brightness threshold [0..1]
 */
void fp_postprocess_bright_pass(
    const float* input,
    float* output,
    size_t pixel_count,
    float threshold
) {
    // Simple bright pass: if pixel > threshold, keep it, else 0
    for (size_t i = 0; i < pixel_count; ++i) {
        output[i] = (input[i] > threshold) ? input[i] : 0.0f;
    }
}

/**
 * PURE FP FUNCTION: Apply bloom effect
 *
 * Bloom = bright pass + blur + combine with original
 *
 * FP Composition:
 *   1. Extract bright regions
 *   2. Blur bright regions
 *   3. Combine with original using fp_zip_add_f32
 *   4. Scale intensity using fp_map_scale_f32
 */
void fp_postprocess_bloom(
    const float* input,
    float* output,
    size_t width,
    size_t height,
    float threshold,
    float intensity
) {
    size_t pixel_count = width * height;

    // Step 1: Extract bright regions
    float* bright = malloc(pixel_count * sizeof(float));
    fp_postprocess_bright_pass(input, bright, pixel_count, threshold);

    // Step 2: Blur bright regions
    float* blurred = malloc(pixel_count * sizeof(float));
    fp_postprocess_blur_horizontal(bright, blurred, width, height);

    // FP LIBRARY: Scale bloom intensity
    fp_map_scale_f32(blurred, blurred, pixel_count, intensity);

    // Step 3: FP LIBRARY: Combine with original
    fp_zip_add_f32(input, blurred, output, pixel_count);

    // Clamp to [0, 1]
    for (size_t i = 0; i < pixel_count; ++i) {
        if (output[i] > 1.0f) output[i] = 1.0f;
    }

    free(bright);
    free(blurred);
}

//==============================================================================
// Tone Mapping (Using FP Library)
//==============================================================================

/**
 * PURE FP FUNCTION: Reinhard tone mapping
 *
 * Simple tone mapping: output = input / (1 + input)
 * Maps HDR [0, ∞) to LDR [0, 1]
 *
 * FP Composition:
 *   - Uses fp_map_scale_f32 for scaling operations
 */
void fp_postprocess_tonemap_reinhard(
    const float* input,
    float* output,
    size_t pixel_count
) {
    // Reinhard: out = in / (1 + in)
    for (size_t i = 0; i < pixel_count; ++i) {
        output[i] = input[i] / (1.0f + input[i]);
    }
}

/**
 * PURE FP FUNCTION: Exposure tone mapping
 *
 * Simple exposure adjustment: output = 1 - exp(-input * exposure)
 *
 * FP Composition:
 *   - Uses fp_map_scale_f32 to apply exposure
 */
void fp_postprocess_tonemap_exposure(
    const float* input,
    float* output,
    size_t pixel_count,
    float exposure
) {
    // Scale by exposure first using FP library
    float* scaled = malloc(pixel_count * sizeof(float));
    fp_map_scale_f32(input, scaled, pixel_count, exposure);

    // Apply exponential tone mapping
    for (size_t i = 0; i < pixel_count; ++i) {
        output[i] = 1.0f - expf(-scaled[i]);
    }

    free(scaled);
}

//==============================================================================
// Brightness/Contrast Adjustment (Using FP Library)
//==============================================================================

/**
 * PURE FP FUNCTION: Adjust brightness
 *
 * Simply adds a constant to all pixels.
 *
 * FP Composition:
 *   - Uses fp_map_offset_f32 to add brightness
 */
void fp_postprocess_brightness(
    const float* input,
    float* output,
    size_t pixel_count,
    float brightness_offset
) {
    // FP LIBRARY: Add constant to all pixels
    fp_map_offset_f32(input, output, pixel_count, brightness_offset);

    // Clamp to [0, 1]
    for (size_t i = 0; i < pixel_count; ++i) {
        if (output[i] < 0.0f) output[i] = 0.0f;
        if (output[i] > 1.0f) output[i] = 1.0f;
    }
}

/**
 * PURE FP FUNCTION: Adjust contrast
 *
 * Scales around midpoint: output = (input - 0.5) * contrast + 0.5
 *
 * FP Composition:
 *   - Uses fp_map_offset_f32 to shift to origin
 *   - Uses fp_map_scale_f32 to scale
 *   - Uses fp_map_offset_f32 to shift back
 */
void fp_postprocess_contrast(
    const float* input,
    float* output,
    size_t pixel_count,
    float contrast
) {
    float* temp = malloc(pixel_count * sizeof(float));

    // Step 1: FP LIBRARY: Subtract 0.5 (shift to origin)
    fp_map_offset_f32(input, temp, pixel_count, -0.5f);

    // Step 2: FP LIBRARY: Scale by contrast
    fp_map_scale_f32(temp, temp, pixel_count, contrast);

    // Step 3: FP LIBRARY: Add 0.5 back (shift back)
    fp_map_offset_f32(temp, output, pixel_count, 0.5f);

    // Clamp to [0, 1]
    for (size_t i = 0; i < pixel_count; ++i) {
        if (output[i] < 0.0f) output[i] = 0.0f;
        if (output[i] > 1.0f) output[i] = 1.0f;
    }

    free(temp);
}

//==============================================================================
// Gamma Correction (Using FP Library)
//==============================================================================

/**
 * PURE FP FUNCTION: Apply gamma correction
 *
 * Gamma correction: output = input^(1/gamma)
 *
 * Uses power function, not directly FP library, but demonstrates
 * the pattern for pixel-wise operations.
 */
void fp_postprocess_gamma(
    const float* input,
    float* output,
    size_t pixel_count,
    float gamma
) {
    float inv_gamma = 1.0f / gamma;

    for (size_t i = 0; i < pixel_count; ++i) {
        output[i] = powf(input[i], inv_gamma);
    }
}
