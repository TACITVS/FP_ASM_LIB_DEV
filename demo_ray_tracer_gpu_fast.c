/**
 * demo_ray_tracer_gpu_fast.c
 *
 * GPU Ray Tracer Demo - Persistent Context (FAST!)
 * This demonstrates the CORRECT way to use GPU rendering:
 * - Initialize ONCE (compile kernel, upload scene)
 * - Render MANY frames (only kernel execution + readback)
 * - Cleanup ONCE
 *
 * Expected performance with GT 730M:
 * - 640×480:  ~125 FPS (8ms per frame)
 * - 1920×1080: ~45 FPS (22ms per frame)
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "include/fp_ray_tracer.h"

int main(void) {
    printf("================================================================\n");
    printf("  FP-ASM Ray Tracer - GPU Persistent Context Demo\n");
    printf("  The CORRECT Way to Use GPU Rendering\n");
    printf("================================================================\n");
    printf("\n");

    // Create scene (3 spheres + ground + 2 lights)
    printf("Creating test scene...\n");

    Scene scene = create_scene();

    // Allocate objects
    scene.spheres = (Sphere*)malloc(3 * sizeof(Sphere));
    scene.n_spheres = 3;

    scene.planes = (Plane*)malloc(1 * sizeof(Plane));
    scene.n_planes = 1;

    scene.lights = (Light*)malloc(2 * sizeof(Light));
    scene.n_lights = 2;

    // Create spheres
    scene.spheres[0] = create_sphere(vec3(0, 0, 3), 1.0f, vec3(1, 0, 0), 500, 0.3f);    // Red
    scene.spheres[1] = create_sphere(vec3(-2, 0, 4), 1.0f, vec3(0, 0, 1), 500, 0.4f);   // Blue
    scene.spheres[2] = create_sphere(vec3(2, 0, 4), 1.0f, vec3(0, 1, 0), 10, 0.5f);     // Green

    // Create ground plane
    scene.planes[0] = create_plane(vec3(0, 1, 0), 1.0f, vec3(1, 1, 0), 1000);  // Yellow ground

    // Create lights
    scene.lights[0] = create_light(vec3(2, 2, 0), vec3(1, 1, 1), 0.6f);
    scene.lights[1] = create_light(vec3(-2, 2, 1), vec3(1, 1, 1), 0.4f);

    // Set ambient light
    scene.ambient = vec3(0.2f, 0.2f, 0.2f);

    // Create camera
    Camera camera = create_camera(
        vec3(0, 1, -3),       // Position
        vec3(0, 0, 4),        // Look at
        vec3(0, 1, 0),        // Up
        60.0f,                // FOV
        4.0f / 3.0f           // Aspect ratio
    );

    printf("\n");
    printf("================================================================\n");
    printf("  INITIALIZATION (ONCE)\n");
    printf("================================================================\n");

    // STEP 1: Initialize GPU context ONCE
    // This compiles the kernel, uploads scene geometry
    clock_t init_start = clock();
    GPUContext* gpu = gpu_init(&scene);
    clock_t init_end = clock();

    if (!gpu) {
        fprintf(stderr, "Failed to initialize GPU context\n");
        free_scene(&scene);
        return 1;
    }

    double init_time = (double)(init_end - init_start) / CLOCKS_PER_SEC;
    printf("Total initialization time: %.3f ms\n", init_time * 1000);

    printf("\n");
    printf("================================================================\n");
    printf("  RENDERING (MANY FRAMES) - HOT PATH!\n");
    printf("================================================================\n");

    // Test 1: 640×480 GPU rendering
    printf("\n--- Test 1: 640×480 GPU Rendering ---\n");
    int width1 = 640;
    int height1 = 480;
    camera.aspect = (float)width1 / height1;

    uint8_t* framebuffer1 = (uint8_t*)malloc(width1 * height1 * 3);

    // STEP 2: Render MULTIPLE frames using persistent context
    // (In a real game, this would be in your game loop)
    printf("Rendering 10 frames to measure average performance...\n");

    clock_t start1 = clock();
    for (int i = 0; i < 10; i++) {
        gpu_render_frame(gpu, &camera, framebuffer1, width1, height1);
    }
    clock_t end1 = clock();

    double total_time1 = (double)(end1 - start1) / CLOCKS_PER_SEC;
    double avg_time1 = total_time1 / 10.0;

    printf("\nResults:\n");
    printf("Total time (10 frames): %.3f seconds\n", total_time1);
    printf("Average per frame:      %.3f ms (%.2f FPS)\n", avg_time1 * 1000, 1.0 / avg_time1);

    save_ppm("output_gpu_fast_640x480.ppm", framebuffer1, width1, height1);
    printf("Saved: output_gpu_fast_640x480.ppm\n");

    free(framebuffer1);

    // Test 2: 1920×1080 GPU rendering (THE BIG TEST!)
    printf("\n--- Test 2: 1920×1080 GPU Rendering ---\n");
    printf("This is the real test - 2.07 million pixels!\n");

    int width2 = 1920;
    int height2 = 1080;
    camera.aspect = (float)width2 / height2;

    uint8_t* framebuffer2 = (uint8_t*)malloc(width2 * height2 * 3);

    // Render multiple frames
    printf("Rendering 10 frames to measure average performance...\n");

    clock_t start2 = clock();
    for (int i = 0; i < 10; i++) {
        gpu_render_frame(gpu, &camera, framebuffer2, width2, height2);
    }
    clock_t end2 = clock();

    double total_time2 = (double)(end2 - start2) / CLOCKS_PER_SEC;
    double avg_time2 = total_time2 / 10.0;

    printf("\nResults:\n");
    printf("Total time (10 frames): %.3f seconds\n", total_time2);
    printf("Average per frame:      %.3f ms (%.2f FPS)\n", avg_time2 * 1000, 1.0 / avg_time2);

    save_ppm("output_gpu_fast_1080p.ppm", framebuffer2, width2, height2);
    printf("Saved: output_gpu_fast_1080p.ppm\n");

    free(framebuffer2);

    printf("\n");
    printf("================================================================\n");
    printf("  CLEANUP (ONCE)\n");
    printf("================================================================\n");

    // STEP 3: Cleanup GPU context ONCE
    gpu_cleanup(gpu);

    // Cleanup scene
    free_scene(&scene);

    // Summary
    printf("\n");
    printf("================================================================\n");
    printf("  PERFORMANCE SUMMARY\n");
    printf("================================================================\n");
    printf("Resolution    |  Avg Frame Time  |  FPS\n");
    printf("----------------------------------------------\n");
    printf("640×480       |  %7.3f ms      |  %.2f\n", avg_time1 * 1000, 1.0 / avg_time1);
    printf("1920×1080     |  %7.3f ms      |  %.2f\n", avg_time2 * 1000, 1.0 / avg_time2);
    printf("\n");
    printf("These are the REAL numbers - no initialization overhead!\n");
    printf("This is how fast your GPU truly is!\n");
    printf("\n");
    printf("Expected with GT 730M (384 CUDA cores):\n");
    printf("- 640×480:    ~8ms  (125 FPS)\n");
    printf("- 1920×1080:  ~22ms (45 FPS)\n");
    printf("\n");
    printf("If you see these numbers, congratulations!\n");
    printf("You have REAL-TIME GPU ray tracing!\n");
    printf("================================================================\n");

    return 0;
}
