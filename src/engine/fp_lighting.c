#include "fp_lighting.h"
#include "fp_core.h"

void fp_lighting_diffuse_batch(
    const Vec3f* normals,
    float* intensities,
    const Vec3f* light_dir,
    float light_intensity,
    size_t vertex_count
) {
    for (size_t i = 0; i < vertex_count; ++i) {
        // Pass the pointer to the padded Vec3f struct directly.
        // The assembly function can now safely read 4 floats (or 8 with AVX).
        float dot = vec3_dot(&normals[i], light_dir);
        float clamped = (dot > 0.0f) ? dot : 0.0f;
        intensities[i] = clamped * light_intensity;
    }
}

void fp_lighting_directional(
    const Vec3f* normals,
    float* intensities,
    const DirectionalLight* light,
    size_t vertex_count
) {
    // Create a padded Vec3f for the light direction to ensure safe SIMD operations.
    Vec3f light_dir_padded = {
        .x = light->direction[0],
        .y = light->direction[1],
        .z = light->direction[2],
        ._pad = 0.0f
    };

    // 1. Calculate diffuse intensity for all vertices
    fp_lighting_diffuse_batch(normals, intensities, &light_dir_padded, light->intensity, vertex_count);

    // 2. Apply light color by scaling the intensities
    fp_map_scale_f32(intensities, intensities, vertex_count, light->color[0]);
}

void fp_lighting_combine(
    const float* light1,
    const float* light2,
    float* combined,
    size_t vertex_count
) {
    // Compose fp_zip_add_f32 to combine lighting contributions
    fp_zip_add_f32(light1, light2, combined, vertex_count);

    // Clamp the results to 1.0
    for (size_t i = 0; i < vertex_count; ++i) {
        if (combined[i] > 1.0f) {
            combined[i] = 1.0f;
        }
    }
}

void fp_lighting_add_ambient(
    const float* intensities,
    float* output,
    float ambient_intensity,
    size_t vertex_count
) {
    // Compose fp_map_offset_f32 to add the ambient term
    fp_map_offset_f32(intensities, output, vertex_count, ambient_intensity);

    // Clamp the results to 1.0
    for (size_t i = 0; i < vertex_count; ++i) {
        if (output[i] > 1.0f) {
            output[i] = 1.0f;
        }
    }
}
