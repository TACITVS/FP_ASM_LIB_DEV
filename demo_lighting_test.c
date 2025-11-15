#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include "fp_core.h"
#include "fp_lighting.h"

#define PI 3.14159265359f
#define TOLERANCE 1e-5f

bool float_equals(float a, float b, float tolerance) {
    return fabsf(a - b) < tolerance;
}

//==============================================================================
// Test 1: Diffuse Lighting - Batch (Multiple Vertices)
//==============================================================================

bool test_diffuse_batch() {
    printf("[Test 1] Diffuse Lighting - Batch\n");
    printf("  Testing: fp_lighting_diffuse_batch (HOT PATH!)\n");

    const int vertex_count = 100;
    float* normals = malloc(vertex_count * 3 * sizeof(float));
    float* intensities = malloc(vertex_count * sizeof(float));

    // All normals pointing up
    for (int i = 0; i < vertex_count; ++i) {
        normals[i * 3 + 0] = 0.0f;
        normals[i * 3 + 1] = 1.0f;
        normals[i * 3 + 2] = 0.0f;
    }

    float light_dir[3] = { 0.0f, 1.0f, 0.0f };
    float light_intensity = 0.7f;

    // FP LIBRARY: Uses fp_fold_dotp_f32 for each vertex
    printf("  → Processing %d vertices...\n", vertex_count);
    fp_lighting_diffuse_batch(normals, intensities, light_dir, light_intensity, vertex_count);

    // All should be 0.7
    bool passed = true;
    for (int i = 0; i < vertex_count; ++i) {
        if (!float_equals(intensities[i], 0.7f, TOLERANCE)) {
            passed = false;
            break;
        }
    }

    printf("  First vertex intensity: %.3f (expected 0.700)\n", intensities[0]);
    printf("  FP Library Calls: %d (fp_fold_dotp_f32)\n", vertex_count);
    printf("  %s\n\n", passed ? "✓ PASS" : "✗ FAIL");

    free(normals);
    free(intensities);

    return passed;
}

//==============================================================================
// Test 2: Directional Light
//==============================================================================

bool test_directional_light() {
    printf("[Test 2] Directional Light\n");
    printf("  Testing: fp_lighting_directional (sun/moon)\n");

    const int vertex_count = 50;
    float* normals = malloc(vertex_count * 3 * sizeof(float));
    float* intensities = malloc(vertex_count * sizeof(float));

    // Normals pointing in different directions
    for (int i = 0; i < vertex_count; ++i) {
        float angle = (float)i / vertex_count * 2.0f * PI;
        normals[i * 3 + 0] = cosf(angle);
        normals[i * 3 + 1] = sinf(angle);
        normals[i * 3 + 2] = 0.0f;
    }

    // Create directional light (sun from above)
    DirectionalLight sun = {
        .direction = { 0.0f, 1.0f, 0.0f },
        .intensity = 1.0f,
        .color = { 1.0f, 1.0f, 1.0f }
    };

    // FP LIBRARY: Uses fp_fold_dotp_f32 + fp_map_scale_f32
    printf("  → Computing lighting for %d vertices...\n", vertex_count);
    fp_lighting_directional(normals, intensities, &sun, vertex_count);

    // Verify some vertices have correct lighting
    bool passed = true;

    // Vertex 12 should have normal ~(0, 1, 0) → full intensity
    if (!float_equals(intensities[12], 1.0f, 0.1f)) {
        passed = false;
    }

    printf("  Sample intensity (vertex pointing up): %.3f\n", intensities[12]);
    printf("  FP Library Functions Used:\n");
    printf("    - fp_fold_dotp_f32 (dot products)\n");
    printf("    - fp_map_scale_f32 (color scaling)\n");
    printf("  %s\n\n", passed ? "✓ PASS" : "✗ FAIL");

    free(normals);
    free(intensities);

    return passed;
}

//==============================================================================
// Test 3: Light Combining
//==============================================================================

bool test_light_combine() {
    printf("[Test 3] Light Combining\n");
    printf("  Testing: fp_lighting_combine (uses fp_zip_add_f32)\n");

    const int vertex_count = 100;
    float* light1 = malloc(vertex_count * sizeof(float));
    float* light2 = malloc(vertex_count * sizeof(float));
    float* combined = malloc(vertex_count * sizeof(float));

    // Fill with test data
    for (int i = 0; i < vertex_count; ++i) {
        light1[i] = 0.3f;
        light2[i] = 0.4f;
    }

    // FP LIBRARY: Uses fp_zip_add_f32 to combine
    printf("  → Combining two light contributions...\n");
    fp_lighting_combine(light1, light2, combined, vertex_count);

    // Expected: 0.3 + 0.4 = 0.7
    float expected = 0.7f;
    bool passed = float_equals(combined[0], expected, TOLERANCE);

    printf("  Light 1: %.1f\n", light1[0]);
    printf("  Light 2: %.1f\n", light2[0]);
    printf("  Combined: %.1f\n", combined[0]);
    printf("  Expected: %.1f\n", expected);
    printf("  FP Library Function: fp_zip_add_f32\n");
    printf("  %s\n\n", passed ? "✓ PASS" : "✗ FAIL");

    free(light1);
    free(light2);
    free(combined);

    return passed;
}

//==============================================================================
// Test 4: Ambient Lighting
//==============================================================================

bool test_ambient() {
    printf("[Test 4] Ambient Lighting\n");
    printf("  Testing: fp_lighting_add_ambient (uses fp_map_offset_f32)\n");

    const int vertex_count = 100;
    float* intensities = malloc(vertex_count * sizeof(float));
    float* output = malloc(vertex_count * sizeof(float));

    // Fill with test data
    for (int i = 0; i < vertex_count; ++i) {
        intensities[i] = 0.5f;
    }

    float ambient = 0.2f;

    // FP LIBRARY: Uses fp_map_offset_f32 to add ambient
    printf("  → Adding ambient light to all vertices...\n");
    fp_lighting_add_ambient(intensities, output, ambient, vertex_count);

    // Expected: 0.5 + 0.2 = 0.7
    float expected = 0.7f;
    bool passed = float_equals(output[0], expected, TOLERANCE);

    printf("  Base intensity: %.1f\n", intensities[0]);
    printf("  Ambient: %.1f\n", ambient);
    printf("  Result: %.1f\n", output[0]);
    printf("  Expected: %.1f\n", expected);
    printf("  FP Library Function: fp_map_offset_f32\n");
    printf("  %s\n\n", passed ? "✓ PASS" : "✗ FAIL");

    free(intensities);
    free(output);

    return passed;
}

//==============================================================================
// Main Test Runner
//==============================================================================

int main(void) {
    printf("==============================================\n");
    printf("   FP-FIRST LIGHTING TEST\n");
    printf("   Using FP Library for All Computations\n");
    printf("==============================================\n\n");

    int passed = 0;
    int total = 4;

    if (test_diffuse_batch()) passed++;
    if (test_directional_light()) passed++;
    if (test_light_combine()) passed++;
    if (test_ambient()) passed++;

    printf("==============================================\n");
    printf("   TEST RESULTS\n");
    printf("==============================================\n");
    printf("Passed: %d/%d tests\n", passed, total);

    if (passed == total) {
        printf("\n✓ ALL TESTS PASSED\n\n");
        printf("FP Library Functions Used:\n");
        printf("  - fp_fold_dotp_f32 (normal · light_dir)\n");
        printf("  - fp_map_scale_f32 (intensity scaling)\n");
        printf("  - fp_zip_add_f32 (combine lights)\n");
        printf("  - fp_map_offset_f32 (add ambient)\n");
        printf("\nPhase 2: Lighting is COMPLETE!\n");
        return 0;
    } else {
        printf("\n✗ SOME TESTS FAILED\n");
        return 1;
    }
}
