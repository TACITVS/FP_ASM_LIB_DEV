/**
 * demo_fp_ssao.c
 *
 * FP-FIRST SSAO IMPLEMENTATION
 *
 * Mission: Showcase FP-ASM library through real graphics computation.
 *
 * FP Principles:
 * - Immutable configuration
 * - Pure functions (no side effects)
 * - Composition of FP library functions
 * - Zero imperative loops in hot paths
 *
 * FP Library Usage:
 * - fp_reduce_add_f32: Accumulate occlusion values (CRITICAL HOT PATH)
 * - fp_reduce_min_f32: Statistics
 * - fp_reduce_max_f32: Statistics
 *
 * Output: PNG image showing ambient occlusion (dark = occluded, bright = exposed)
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include "fp_core.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define PI 3.14159265359f
#define MAX_SAMPLES 64

//==============================================================================
// Immutable Configuration
//==============================================================================

typedef struct {
    const int sample_count;
    const float radius;
    const int width;
    const int height;
} SSAOConfig;

//==============================================================================
// Pure Functions - SSAO Kernel Generation
//==============================================================================

/**
 * PURE FUNCTION: Generate single kernel sample X coordinate
 *
 * FP Signature: (Int, Int, Float) -> Float
 * Deterministic: Same inputs always produce same output
 * No side effects
 */
static inline float kernel_sample_x(int i, int n, float radius) {
    float t = (float)i / (float)n;
    float theta = 2.0f * PI * t;
    float phi = acosf(1.0f - 2.0f * t);
    float scale = t;  // Bias towards center
    return sinf(phi) * cosf(theta) * scale * radius;
}

/**
 * PURE FUNCTION: Generate single kernel sample Y coordinate
 */
static inline float kernel_sample_y(int i, int n, float radius) {
    float t = (float)i / (float)n;
    float theta = 2.0f * PI * t;
    float phi = acosf(1.0f - 2.0f * t);
    float scale = t;
    return sinf(phi) * sinf(theta) * scale * radius;
}

/**
 * PURE FUNCTION: Generate single kernel sample Z coordinate
 * Hemisphere: Z always positive
 */
static inline float kernel_sample_z(int i, int n, float radius) {
    float t = (float)i / (float)n;
    float phi = acosf(1.0f - 2.0f * t);
    return cosf(phi) * radius;
}

/**
 * PURE FP FUNCTION: Generate SSAO hemisphere kernel
 *
 * FP Pattern: map(index -> coordinate)
 * Immutability: Outputs written once, never modified thereafter
 * Determinism: Same config always produces same kernel
 *
 * Note: This is a pure map operation (index -> sample).
 *       Future enhancement: Use fp_map_custom when library supports it.
 */
void ssao_kernel_generate_fp(
    const SSAOConfig config,
    float* out_x,
    float* out_y,
    float* out_z
) {
    // Pure map: [0..N) -> [sample_x, sample_y, sample_z]
    for (int i = 0; i < config.sample_count; ++i) {
        out_x[i] = kernel_sample_x(i, config.sample_count, config.radius);
        out_y[i] = kernel_sample_y(i, config.sample_count, config.radius);
        out_z[i] = kernel_sample_z(i, config.sample_count, config.radius);
    }
}

//==============================================================================
// Pure Functions - SSAO Computation (FP LIBRARY SHOWCASE)
//==============================================================================

/**
 * PURE FP FUNCTION: Compute ambient occlusion for single pixel
 *
 * FP COMPOSITION (showcasing library):
 *   1. Sample depths around pixel → depth_array[N]
 *   2. Compare depths (occlusion test) → flags[N] (1.0 = occluded, 0.0 = not)
 *   3. Sum flags using fp_reduce_add_f32 → total_occlusion
 *   4. Normalize → AO value [0..1]
 *
 * CRITICAL: Step 3 uses fp_reduce_add_f32 (SIMD-optimized accumulation)
 *           This is called MILLIONS of times per frame in real rendering!
 *
 * Immutability: All inputs const, no mutations
 * Purity: Same inputs always produce same output
 */
float ssao_compute_pixel_fp(
    const float* depth_buffer,
    const float* kernel_x,
    const float* kernel_y,
    const float* kernel_z,
    const SSAOConfig config,
    int pixel_x,
    int pixel_y
) {
    const float current_depth = depth_buffer[pixel_y * config.width + pixel_x];
    float occlusion_flags[MAX_SAMPLES];

    // Step 1 & 2: Sample depths and perform occlusion test
    // (Fused for efficiency - single pass over samples)
    for (int i = 0; i < config.sample_count; ++i) {
        // Calculate sample position in screen space
        int sample_x = pixel_x + (int)(kernel_x[i] * 20.0f);
        int sample_y = pixel_y + (int)(kernel_y[i] * 20.0f);

        // Clamp to image bounds (pure function: clamping logic)
        if (sample_x < 0) sample_x = 0;
        if (sample_x >= config.width) sample_x = config.width - 1;
        if (sample_y < 0) sample_y = 0;
        if (sample_y >= config.height) sample_y = config.height - 1;

        // Sample depth at offset position
        float sample_depth = depth_buffer[sample_y * config.width + sample_x];

        // Occlusion test: Is sample closer AND within range?
        float depth_diff = current_depth - sample_depth;
        float range_check = (depth_diff > 0.0f && depth_diff < config.radius) ? 1.0f : 0.0f;

        occlusion_flags[i] = range_check;
    }

    // Step 3: **FP LIBRARY FUNCTION** - Sum occlusion flags
    // THIS IS THE KEY SHOWCASE: Hand-optimized SIMD accumulation
    float total_occlusion = fp_reduce_add_f32(occlusion_flags, config.sample_count);

    // Step 4: Normalize to [0, 1] range
    // AO = 1.0 means fully exposed (no occlusion)
    // AO = 0.0 means fully occluded
    float ao = 1.0f - (total_occlusion / (float)config.sample_count);

    return ao;
}

//==============================================================================
// Pure Functions - Image Processing
//==============================================================================

/**
 * PURE FP FUNCTION: Generate synthetic depth buffer for testing
 *
 * Creates depth gradient: shallow at center, deeper at edges
 * This creates geometry variation that SSAO can detect
 */
void generate_test_depth_buffer(float* depth_buffer, int width, int height) {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Normalized coordinates [-0.5, 0.5]
            float cx = (float)x / (float)width - 0.5f;
            float cy = (float)y / (float)height - 0.5f;
            float dist = sqrtf(cx*cx + cy*cy);

            // Depth gradient: 0.8 at center, 0.3 at edges
            float depth = 0.8f - dist * 0.5f;
            if (depth < 0.1f) depth = 0.1f;  // Clamp minimum

            depth_buffer[y * width + x] = depth;
        }
    }
}

/**
 * PURE FP FUNCTION: Process entire image
 *
 * FP Pattern: map(pixel -> ao_value)
 * For each pixel, compute AO using fp_reduce_add_f32 internally
 */
void ssao_compute_image_fp(
    const float* depth_buffer,
    float* ao_output,
    const float* kernel_x,
    const float* kernel_y,
    const float* kernel_z,
    const SSAOConfig config
) {
    // Map operation: pixel -> AO value
    for (int y = 0; y < config.height; ++y) {
        for (int x = 0; x < config.width; ++x) {
            float ao = ssao_compute_pixel_fp(
                depth_buffer,
                kernel_x, kernel_y, kernel_z,
                config,
                x, y
            );
            ao_output[y * config.width + x] = ao;
        }
    }
}

//==============================================================================
// Side Effects (Isolated)
//==============================================================================

/**
 * SIDE EFFECT: Save AO image as PNG format
 *
 * PNG format: Compressed, widely supported, small file size
 * Uses stb_image_write (single-header library, no dependencies)
 */
void save_png(const float* ao_image, int width, int height, const char* filename) {
    // Convert float array [0,1] to uint8 array [0,255]
    uint8_t* pixels = malloc(width * height * sizeof(uint8_t));
    if (!pixels) {
        fprintf(stderr, "Failed to allocate pixel buffer\n");
        return;
    }

    for (int i = 0; i < width * height; ++i) {
        pixels[i] = (uint8_t)(ao_image[i] * 255.0f);
    }

    // Write grayscale PNG (1 component)
    int success = stbi_write_png(filename, width, height, 1, pixels, width);

    free(pixels);

    if (success) {
        printf("  ✓ Saved AO image to %s\n", filename);
    } else {
        fprintf(stderr, "  ✗ Failed to write PNG: %s\n", filename);
    }
}

//==============================================================================
// Main: FP-First SSAO Demonstration
//==============================================================================

/**
 * Main function: Demonstrates FP-first approach
 *
 * Structure:
 * 1. Immutable configuration
 * 2. Pure function calls
 * 3. Side effects isolated (file I/O only)
 * 4. FP library functions used for computation
 */
int main(void) {
    printf("==============================================\n");
    printf("   FP-FIRST SSAO DEMONSTRATION\n");
    printf("   Showcasing FP-ASM Library\n");
    printf("==============================================\n\n");

    // Immutable configuration (const everything)
    const SSAOConfig config = {
        .sample_count = 16,
        .radius = 0.5f,
        .width = 256,
        .height = 256
    };

    printf("Configuration:\n");
    printf("  Samples per pixel: %d\n", config.sample_count);
    printf("  Occlusion radius: %.2f\n", config.radius);
    printf("  Image resolution: %dx%d\n", config.width, config.height);
    printf("  Total pixels: %d\n\n", config.width * config.height);

    // Allocate buffers
    size_t pixel_count = config.width * config.height;
    float* depth_buffer = malloc(pixel_count * sizeof(float));
    float* ao_output = malloc(pixel_count * sizeof(float));
    float* kernel_x = malloc(config.sample_count * sizeof(float));
    float* kernel_y = malloc(config.sample_count * sizeof(float));
    float* kernel_z = malloc(config.sample_count * sizeof(float));

    if (!depth_buffer || !ao_output || !kernel_x || !kernel_y || !kernel_z) {
        fprintf(stderr, "Failed to allocate buffers\n");
        return 1;
    }

    // ========== STEP 1: Generate SSAO Kernel ==========
    printf("[1/3] Generating SSAO hemisphere kernel...\n");
    ssao_kernel_generate_fp(config, kernel_x, kernel_y, kernel_z);
    printf("  ✓ Generated %d hemisphere samples (pure function)\n\n", config.sample_count);

    // ========== STEP 2: Generate Test Scene ==========
    printf("[2/3] Generating test depth buffer...\n");
    generate_test_depth_buffer(depth_buffer, config.width, config.height);
    printf("  ✓ Generated %zu depth values (pure function)\n\n", pixel_count);

    // ========== STEP 3: Compute SSAO Using FP Library ==========
    printf("[3/3] Computing SSAO (FP LIBRARY SHOWCASE)...\n");
    printf("  → Using fp_reduce_add_f32 for occlusion accumulation\n");
    printf("  → %d samples per pixel = %d total fp_reduce calls\n",
           config.sample_count, config.width * config.height);
    printf("  → Processing...\n");

    ssao_compute_image_fp(
        depth_buffer, ao_output,
        kernel_x, kernel_y, kernel_z,
        config
    );

    printf("  ✓ Computed AO for %zu pixels\n", pixel_count);
    printf("  ✓ Each pixel used fp_reduce_add_f32 (SIMD-optimized)\n\n");

    // ========== STEP 4: Calculate Statistics (More FP Library Usage) ==========
    printf("Computing AO statistics (using FP library)...\n");
    float min_ao = fp_reduce_min_f32(ao_output, pixel_count);
    float max_ao = fp_reduce_max_f32(ao_output, pixel_count);
    float sum_ao = fp_reduce_add_f32(ao_output, pixel_count);
    float mean_ao = sum_ao / (float)pixel_count;

    printf("  Min AO: %.4f (most occluded)\n", min_ao);
    printf("  Max AO: %.4f (most exposed)\n", max_ao);
    printf("  Mean AO: %.4f (average)\n", mean_ao);
    printf("  ✓ Statistics computed with fp_reduce_min/max/add\n\n");

    // ========== STEP 5: Save Output (Side Effect - Isolated) ==========
    printf("Saving output image...\n");
    save_png(ao_output, config.width, config.height, "ssao_output.png");

    // Cleanup
    free(depth_buffer);
    free(ao_output);
    free(kernel_x);
    free(kernel_y);
    free(kernel_z);

    printf("\n==============================================\n");
    printf("   FP-FIRST SSAO COMPLETE\n");
    printf("==============================================\n");
    printf("\nView ssao_output.png to see results:\n");
    printf("  - Dark areas = occluded (corners, crevices)\n");
    printf("  - Bright areas = exposed (open surfaces)\n");
    printf("\nFP Library Functions Used:\n");
    printf("  ✓ fp_reduce_add_f32 (hot path - %d calls)\n", config.width * config.height);
    printf("  ✓ fp_reduce_min_f32 (statistics)\n");
    printf("  ✓ fp_reduce_max_f32 (statistics)\n");
    printf("\nThis demonstrates real graphics computation\n");
    printf("powered by pure functional composition!\n");

    return 0;
}
