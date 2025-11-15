/**
 * demo_ray_tracer_simple.c
 *
 * Simple test of the ray tracer - generates a basic scene
 * Tests both real-time and offline modes
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "include/fp_ray_tracer.h"

// Create a simple test scene: 3 spheres on a ground plane
Scene create_test_scene(void) {
    Scene scene = create_scene();

    // Allocate objects
    scene.spheres = (Sphere*)malloc(3 * sizeof(Sphere));
    scene.n_spheres = 3;

    scene.planes = (Plane*)malloc(1 * sizeof(Plane));
    scene.n_planes = 1;

    scene.lights = (Light*)malloc(2 * sizeof(Light));
    scene.n_lights = 2;

    // Sphere 1: Red diffuse
    scene.spheres[0] = create_sphere(
        vec3(-1.0f, 0.5f, -3.0f),  // center
        0.5f,                       // radius
        vec3(0.9f, 0.2f, 0.2f),    // color (red)
        50.0f,                      // specular
        0.0f                        // not reflective
    );

    // Sphere 2: Green shiny
    scene.spheres[1] = create_sphere(
        vec3(0.0f, 0.6f, -3.5f),
        0.6f,
        vec3(0.2f, 0.9f, 0.3f),    // color (green)
        200.0f,                     // very shiny
        0.0f
    );

    // Sphere 3: Blue reflective
    scene.spheres[2] = create_sphere(
        vec3(1.2f, 0.4f, -2.8f),
        0.4f,
        vec3(0.2f, 0.5f, 0.9f),    // color (blue)
        100.0f,
        0.5f                        // 50% reflective
    );

    // Ground plane
    scene.planes[0] = create_plane(
        vec3(0, 1, 0),              // normal (pointing up)
        0.0f,                       // distance from origin
        vec3(0.6f, 0.6f, 0.6f),    // color (gray)
        10.0f                       // slightly shiny
    );

    // Light 1: Main light (white, from above and side)
    scene.lights[0] = create_light(
        vec3(2.0f, 3.0f, -1.0f),
        vec3(1.0f, 1.0f, 1.0f),
        0.8f
    );

    // Light 2: Fill light (blue tint, from other side)
    scene.lights[1] = create_light(
        vec3(-2.0f, 2.0f, -2.0f),
        vec3(0.6f, 0.7f, 1.0f),
        0.4f
    );

    // Ambient light
    scene.ambient = vec3(0.05f, 0.05f, 0.08f);

    return scene;
}

int main(void) {
    printf("================================================================\n");
    printf("  FP-ASM Ray Tracer - Simple Test\n");
    printf("================================================================\n\n");

    // Create scene
    printf("Creating test scene (3 spheres + ground plane + 2 lights)...\n");
    Scene scene = create_test_scene();

    // Create camera
    Camera camera = create_camera(
        vec3(0.0f, 1.2f, 0.0f),     // position
        vec3(0.0f, 0.6f, -3.0f),    // look_at (center of scene)
        vec3(0.0f, 1.0f, 0.0f),     // up
        60.0f,                       // fov
        16.0f / 9.0f                 // aspect ratio
    );

    // ==================================================================
    // Test 1: Small real-time render (400x300)
    // ==================================================================
    printf("\n--- Test 1: Real-Time Mode (400x300) ---\n");

    int rt_width = 400;
    int rt_height = 300;
    uint8_t* framebuffer = (uint8_t*)malloc(rt_width * rt_height * 3);

    camera.aspect = (float)rt_width / rt_height;

    clock_t start = clock();
    render_realtime(&scene, &camera, framebuffer, rt_width, rt_height);
    clock_t end = clock();

    double rt_time = (double)(end - start) / CLOCKS_PER_SEC;
    double fps = 1.0 / rt_time;

    printf("Render time: %.3f seconds (%.2f FPS)\n", rt_time, fps);

    // Save as PPM
    save_ppm("output_realtime.ppm", framebuffer, rt_width, rt_height);
    free(framebuffer);

    // ==================================================================
    // Test 2: Offline render (800x600, with reflections)
    // ==================================================================
    printf("\n--- Test 2: Offline Mode (800x600, reflections) ---\n");

    RenderSettings settings;
    settings.width = 800;
    settings.height = 600;
    settings.samples_per_pixel = 1;  // No AA for now
    settings.max_bounces = 3;         // Enable reflections
    settings.shadows = true;

    camera.aspect = (float)settings.width / settings.height;

    start = clock();
    render_offline(&scene, &camera, "output_offline.ppm", &settings);
    end = clock();

    double offline_time = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Render time: %.3f seconds\n", offline_time);

    // ==================================================================
    // Test 3: High-quality render (1920x1080, 2x AA)
    // ==================================================================
    printf("\n--- Test 3: High-Quality Mode (1920x1080, 2x AA) ---\n");

    settings.width = 1920;
    settings.height = 1080;
    settings.samples_per_pixel = 2;   // 2x2 = 4 samples per pixel
    settings.max_bounces = 5;          // More bounces
    settings.shadows = true;

    camera.aspect = (float)settings.width / settings.height;

    start = clock();
    render_offline(&scene, &camera, "output_hq.ppm", &settings);
    end = clock();

    double hq_time = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Render time: %.3f seconds\n", hq_time);

    // ==================================================================
    // Summary
    // ==================================================================
    printf("\n================================================================\n");
    printf("  Summary\n");
    printf("================================================================\n");
    printf("Real-time (400x300):      %.3fs (%.2f FPS)\n", rt_time, fps);
    printf("Offline (800x600):        %.3fs\n", offline_time);
    printf("High-quality (1920x1080): %.3fs\n", hq_time);
    printf("\nOutput files:\n");
    printf("  - output_realtime.ppm (400x300, fast preview)\n");
    printf("  - output_offline.ppm  (800x600, reflections)\n");
    printf("  - output_hq.ppm       (1920x1080, 4xAA)\n");
    printf("\nTo view: Use image viewer supporting PPM format\n");
    printf("  Windows: IrfanView, XnView, GIMP\n");
    printf("  Convert: magick output.ppm output.png\n");
    printf("================================================================\n");

    // Cleanup
    free_scene(&scene);

    return 0;
}
