/**
 * demo_fp_graphics_showcase.c
 *
 * GRAPHICAL DEMO: FP-First Graphics Engine
 *
 * Renders a 3D scene to a PPM image showcasing:
 * - FP-first lighting (directional + point lights)
 * - FP-first post-processing (bloom + tone mapping + gamma)
 * - Pure functional composition throughout
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "fp_core.h"

#define WIDTH 800
#define HEIGHT 600
#define PI 3.14159265359f

// Include FP-first graphics modules inline
#include "src/engine/fp_graphics_postprocess.c"

//==============================================================================
// Simple Ray-Sphere Intersection (Pure Function)
//==============================================================================

typedef struct {
    float origin[3];
    float direction[3];
} Ray;

typedef struct {
    float center[3];
    float radius;
} Sphere;

bool ray_sphere_intersect(const Ray* ray, const Sphere* sphere, float* t_out, float hit_normal[3]) {
    float oc[3] = {
        ray->origin[0] - sphere->center[0],
        ray->origin[1] - sphere->center[1],
        ray->origin[2] - sphere->center[2]
    };

    float a = fp_fold_dotp_f32(ray->direction, ray->direction, 3);
    float b = 2.0f * fp_fold_dotp_f32(oc, ray->direction, 3);
    float c = fp_fold_dotp_f32(oc, oc, 3) - sphere->radius * sphere->radius;
    float discriminant = b * b - 4 * a * c;

    if (discriminant < 0) return false;

    float t = (-b - sqrtf(discriminant)) / (2.0f * a);
    if (t < 0.001f) return false;

    *t_out = t;

    // Calculate hit point and normal
    float hit[3] = {
        ray->origin[0] + t * ray->direction[0],
        ray->origin[1] + t * ray->direction[1],
        ray->origin[2] + t * ray->direction[2]
    };

    hit_normal[0] = (hit[0] - sphere->center[0]) / sphere->radius;
    hit_normal[1] = (hit[1] - sphere->center[1]) / sphere->radius;
    hit_normal[2] = (hit[2] - sphere->center[2]) / sphere->radius;

    return true;
}

//==============================================================================
// FP-First Lighting (Using FP Library)
//==============================================================================

float compute_diffuse_lighting(const float normal[3], const float light_dir[3]) {
    float dot = fp_fold_dotp_f32(normal, light_dir, 3);
    return (dot > 0.0f) ? dot : 0.0f;
}

float compute_point_light(const float hit_pos[3], const float normal[3],
                          const float light_pos[3], float intensity) {
    float light_vec[3] = {
        light_pos[0] - hit_pos[0],
        light_pos[1] - hit_pos[1],
        light_pos[2] - hit_pos[2]
    };

    float dist_sq = fp_fold_dotp_f32(light_vec, light_vec, 3);
    float dist = sqrtf(dist_sq);

    if (dist < 0.001f) return 0.0f;

    float light_dir[3] = {
        light_vec[0] / dist,
        light_vec[1] / dist,
        light_vec[2] / dist
    };

    float diffuse = compute_diffuse_lighting(normal, light_dir);
    float attenuation = 1.0f / (1.0f + dist_sq * 0.1f);

    return diffuse * intensity * attenuation;
}

//==============================================================================
// Scene Rendering
//==============================================================================

void render_scene(float* framebuffer) {
    printf("Rendering scene (%dx%d)...\n", WIDTH, HEIGHT);

    // Camera setup
    float camera_pos[3] = { 0.0f, 0.0f, 5.0f };

    // Sphere setup
    Sphere sphere = {
        .center = { 0.0f, 0.0f, 0.0f },
        .radius = 1.0f
    };

    // Lights
    float sun_dir[3] = { -0.5f, 1.0f, 0.5f };
    float sun_length = sqrtf(fp_fold_dotp_f32(sun_dir, sun_dir, 3));
    sun_dir[0] /= sun_length;
    sun_dir[1] /= sun_length;
    sun_dir[2] /= sun_length;

    float point_light_pos[3] = { 2.0f, 1.0f, 3.0f };
    float ambient = 0.1f;

    // Render each pixel
    for (int y = 0; y < HEIGHT; y++) {
        if (y % 100 == 0) printf("  Row %d/%d\n", y, HEIGHT);

        for (int x = 0; x < WIDTH; x++) {
            // Create ray
            float u = (x - WIDTH * 0.5f) / (WIDTH * 0.5f);
            float v = (HEIGHT * 0.5f - y) / (HEIGHT * 0.5f);

            Ray ray = {
                .origin = { camera_pos[0], camera_pos[1], camera_pos[2] },
                .direction = { u, v, -1.0f }
            };

            // Normalize ray direction
            float len = sqrtf(fp_fold_dotp_f32(ray.direction, ray.direction, 3));
            ray.direction[0] /= len;
            ray.direction[1] /= len;
            ray.direction[2] /= len;

            // Test intersection
            float t;
            float normal[3];

            if (ray_sphere_intersect(&ray, &sphere, &t, normal)) {
                // Hit! Calculate lighting using FP library
                float hit_pos[3] = {
                    ray.origin[0] + t * ray.direction[0],
                    ray.origin[1] + t * ray.direction[1],
                    ray.origin[2] + t * ray.direction[2]
                };

                // FP-FIRST: Directional light (sun)
                float sun_intensity = compute_diffuse_lighting(normal, sun_dir) * 0.8f;

                // FP-FIRST: Point light
                float point_intensity = compute_point_light(hit_pos, normal, point_light_pos, 1.0f);

                // FP-FIRST: Combine lights + ambient
                float total_intensity = sun_intensity + point_intensity + ambient;
                if (total_intensity > 1.0f) total_intensity = 1.0f;

                // Store in framebuffer (HDR - can be > 1.0)
                int idx = y * WIDTH + x;
                framebuffer[idx] = sun_intensity + point_intensity * 2.0f + ambient; // Allow HDR
            } else {
                // Sky gradient
                float t_sky = 0.5f * (ray.direction[1] + 1.0f);
                int idx = y * WIDTH + x;
                framebuffer[idx] = 0.3f + t_sky * 0.2f;
            }
        }
    }

    printf("Rendering complete!\n\n");
}

//==============================================================================
// Main
//==============================================================================

int main(void) {
    printf("==============================================\n");
    printf("   FP-FIRST GRAPHICS ENGINE SHOWCASE\n");
    printf("   Demonstrating Pure Functional Graphics\n");
    printf("==============================================\n\n");

    // Allocate framebuffer
    size_t pixel_count = WIDTH * HEIGHT;
    float* framebuffer = malloc(pixel_count * sizeof(float));
    float* processed = malloc(pixel_count * sizeof(float));
    float* temp = malloc(pixel_count * sizeof(float));

    if (!framebuffer || !processed || !temp) {
        fprintf(stderr, "Failed to allocate framebuffer\n");
        return 1;
    }

    // Step 1: Render scene with FP-first lighting
    printf("PHASE 1: Rendering with FP-First Lighting\n");
    printf("  - Directional light (sun)\n");
    printf("  - Point light (bulb)\n");
    printf("  - Ambient lighting\n");
    printf("  - Using fp_fold_dotp_f32 for all dot products\n\n");
    render_scene(framebuffer);

    // Step 2: Apply FP-first post-processing
    printf("PHASE 2: Applying FP-First Post-Processing\n");

    // Extract bright regions (bloom)
    printf("  [1/3] Bloom effect (bright pass)...\n");
    fp_postprocess_bright_pass(framebuffer, temp, pixel_count, 0.8f);

    // FP LIBRARY: Scale bloom intensity
    fp_map_scale_f32(temp, temp, pixel_count, 0.3f);

    // FP LIBRARY: Combine with original
    fp_zip_add_f32(framebuffer, temp, processed, pixel_count);

    // Tone mapping (Reinhard)
    printf("  [2/3] Tone mapping (Reinhard HDR -> LDR)...\n");
    fp_postprocess_tonemap_reinhard(processed, temp, pixel_count);

    // Gamma correction
    printf("  [3/3] Gamma correction (2.2)...\n");
    fp_postprocess_gamma(temp, processed, pixel_count, 2.2f);

    printf("Post-processing complete!\n\n");

    // Step 3: Output PPM image
    printf("PHASE 3: Writing output image\n");
    FILE* fp = fopen("fp_graphics_showcase.ppm", "wb");
    if (!fp) {
        fprintf(stderr, "Failed to open output file\n");
        return 1;
    }

    fprintf(fp, "P6\n%d %d\n255\n", WIDTH, HEIGHT);

    for (size_t i = 0; i < pixel_count; i++) {
        unsigned char val = (unsigned char)(processed[i] * 255.0f);
        fputc(val, fp);  // R
        fputc(val, fp);  // G
        fputc(val, fp);  // B
    }

    fclose(fp);

    printf("  Output: fp_graphics_showcase.ppm\n\n");

    // Summary
    printf("==============================================\n");
    printf("   SHOWCASE COMPLETE!\n");
    printf("==============================================\n");
    printf("FP Library Functions Used:\n");
    printf("  - fp_fold_dotp_f32 (all dot products)\n");
    printf("  - fp_map_scale_f32 (bloom intensity)\n");
    printf("  - fp_zip_add_f32 (combine bloom)\n");
    printf("  - fp_postprocess_bright_pass (bloom)\n");
    printf("  - fp_postprocess_tonemap_reinhard (HDR)\n");
    printf("  - fp_postprocess_gamma (sRGB)\n");
    printf("\nThe FP-first graphics engine renders beautiful\n");
    printf("images using pure functional composition!\n");
    printf("\nView the image: fp_graphics_showcase.ppm\n");
    printf("==============================================\n");

    free(framebuffer);
    free(processed);
    free(temp);

    return 0;
}
