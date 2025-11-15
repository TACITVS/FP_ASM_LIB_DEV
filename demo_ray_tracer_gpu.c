/**
 * demo_ray_tracer_gpu.c
 *
 * GPU Ray Tracer Demo - OpenCL Acceleration
 * Tests the GPU backend using OpenCL for massive parallelism
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "include/fp_ray_tracer.h"

int main(void) {
    printf("================================================================\n");
    printf("  FP-ASM Ray Tracer - GPU OpenCL Demo\n");
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

    // Test 1: 640x480 GPU rendering
    printf("\n--- Test 1: GPU Rendering (640x480) ---\n");
    int width1 = 640;
    int height1 = 480;
    camera.aspect = (float)width1 / height1;

    uint8_t* framebuffer1 = (uint8_t*)malloc(width1 * height1 * 3);

    clock_t start1 = clock();
    render_frame(&scene, &camera, framebuffer1, width1, height1, RENDER_BACKEND_GPU_OPENCL);
    clock_t end1 = clock();

    double time1 = (double)(end1 - start1) / CLOCKS_PER_SEC;
    printf("Render time: %.3f seconds (%.2f FPS)\n", time1, 1.0 / time1);

    save_ppm("output_gpu_640x480.ppm", framebuffer1, width1, height1);
    printf("Saved: output_gpu_640x480.ppm\n");

    free(framebuffer1);

    // Test 2: 1920x1080 GPU rendering (THE BIG TEST!)
    printf("\n--- Test 2: GPU Rendering (1920x1080) ---\n");
    printf("This is the real test - 2.07 million pixels!\n");

    int width2 = 1920;
    int height2 = 1080;
    camera.aspect = (float)width2 / height2;

    uint8_t* framebuffer2 = (uint8_t*)malloc(width2 * height2 * 3);

    clock_t start2 = clock();
    render_frame(&scene, &camera, framebuffer2, width2, height2, RENDER_BACKEND_GPU_OPENCL);
    clock_t end2 = clock();

    double time2 = (double)(end2 - start2) / CLOCKS_PER_SEC;
    printf("Render time: %.3f seconds (%.2f FPS)\n", time2, 1.0 / time2);

    save_ppm("output_gpu_1080p.ppm", framebuffer2, width2, height2);
    printf("Saved: output_gpu_1080p.ppm\n");

    free(framebuffer2);

    // Cleanup
    free_scene(&scene);

    // Summary
    printf("\n");
    printf("================================================================\n");
    printf("  GPU Rendering Complete!\n");
    printf("================================================================\n");
    printf("640x480:      %.3fs (%.2f FPS)\n", time1, 1.0 / time1);
    printf("1920x1080:    %.3fs (%.2f FPS)\n", time2, 1.0 / time2);
    printf("\n");
    printf("Expected GPU performance on GT 730M (384 cores):\n");
    printf("- 640x480:    ~180 FPS (5.5ms)\n");
    printf("- 1920x1080:  ~30-60 FPS (16-33ms)\n");
    printf("\n");
    printf("If you see these numbers, congratulations!\n");
    printf("You have real-time GPU ray tracing on 2013 hardware!\n");
    printf("================================================================\n");

    return 0;
}
