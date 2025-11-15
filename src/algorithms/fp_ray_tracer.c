/**
 * fp_ray_tracer.c
 *
 * Real-Time Ray Tracer Implementation
 * Phase 1: Scalar version (working baseline) ✅
 * Phase 2: Multithreaded CPU version (current)
 * Phase 3: OpenCL GPU version (next)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <time.h>
#include "../../include/fp_ray_tracer.h"

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else
#include <pthread.h>
#include <unistd.h>
#endif

// OpenCL support (conditional compilation)
#ifdef USE_OPENCL
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif
#endif

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define EPSILON 0.001f

// ============================================================================
// Vector Math Operations
// ============================================================================

// NOTE: These are now implemented in fp_vector_ops.c and declared in fp_core.h
// This file now uses the canonical implementations.

// ============================================================================
// Ray Intersection
// ============================================================================

bool ray_sphere_intersect(const Ray* ray, const Sphere* sphere, Hit* hit) {
    // Ray-sphere intersection using quadratic formula
    // (origin + t*direction - center)² = radius²

    Vec3f oc;
    vec3_sub(&oc, &ray->origin, &sphere->center);

    float a = vec3_dot(&ray->direction, &ray->direction);
    float b = 2.0f * vec3_dot(&oc, &ray->direction);
    float c = vec3_dot(&oc, &oc) - sphere->radius * sphere->radius;

    float discriminant = b * b - 4.0f * a * c;

    if (discriminant < 0) {
        return false;  // No intersection
    }

    // Find nearest positive t
    float sqrt_disc = sqrtf(discriminant);
    float t = (-b - sqrt_disc) / (2.0f * a);

    if (t < EPSILON) {
        t = (-b + sqrt_disc) / (2.0f * a);
        if (t < EPSILON) {
            return false;  // Behind ray origin
        }
    }

    // Fill hit record
    hit->hit = true;
    hit->t = t;
    Vec3f scaled_direction;
    vec3_scale(&scaled_direction, &ray->direction, t);
    vec3_add(&hit->point, &ray->origin, &scaled_direction);

    Vec3f sub_result;
    vec3_sub(&sub_result, &hit->point, &sphere->center);
    vec3_normalize(&hit->normal, &sub_result);
    
    hit->color = sphere->color;
    hit->specular = sphere->specular;
    hit->reflective = sphere->reflective;

    return true;
}

bool ray_plane_intersect(const Ray* ray, const Plane* plane, Hit* hit) {
    // Ray-plane intersection
    // dot(normal, point) + distance = 0

    float denom = vec3_dot(&plane->normal, &ray->direction);

    if (fabsf(denom) < EPSILON) {
        return false;  // Ray parallel to plane
    }

    float t = -(vec3_dot(&plane->normal, &ray->origin) + plane->distance) / denom;

    if (t < EPSILON) {
        return false;  // Behind ray origin
    }

    // Fill hit record
    hit->hit = true;
    hit->t = t;
    Vec3f scaled_direction;
    vec3_scale(&scaled_direction, &ray->direction, t);
    vec3_add(&hit->point, &ray->origin, &scaled_direction);
    hit->normal = plane->normal;
    hit->color = plane->color;
    hit->specular = plane->specular;
    hit->reflective = 0.0f;  // Planes don't reflect

    return true;
}

bool ray_scene_intersect(const Ray* ray, const Scene* scene, Hit* hit) {
    bool any_hit = false;
    float closest_t = FLT_MAX;
    Hit temp_hit;

    // Test all spheres
    for (int i = 0; i < scene->n_spheres; i++) {
        if (ray_sphere_intersect(ray, &scene->spheres[i], &temp_hit)) {
            if (temp_hit.t < closest_t) {
                closest_t = temp_hit.t;
                *hit = temp_hit;
                any_hit = true;
            }
        }
    }

    // Test all planes
    for (int i = 0; i < scene->n_planes; i++) {
        if (ray_plane_intersect(ray, &scene->planes[i], &temp_hit)) {
            if (temp_hit.t < closest_t) {
                closest_t = temp_hit.t;
                *hit = temp_hit;
                any_hit = true;
            }
        }
    }

    hit->hit = any_hit;
    return any_hit;
}

// ============================================================================
// Shading
// ============================================================================

bool is_light_visible(const Scene* scene, const Vec3f* point, const Vec3f* light_pos) {
    // Cast shadow ray from point to light
    Vec3f light_dir;
    vec3_sub(&light_dir, light_pos, point);
    float light_dist = vec3_length(&light_dir);
    vec3_normalize(&light_dir, &light_dir);

    Ray shadow_ray;
    Vec3f scaled_light_dir;
    vec3_scale(&scaled_light_dir, &light_dir, EPSILON);
    vec3_add(&shadow_ray.origin, point, &scaled_light_dir);  // Offset to avoid self-intersection
    shadow_ray.direction = light_dir;

    Hit shadow_hit;

    // Check if anything blocks the light
    for (int i = 0; i < scene->n_spheres; i++) {
        if (ray_sphere_intersect(&shadow_ray, &scene->spheres[i], &shadow_hit)) {
            if (shadow_hit.t < light_dist) {
                return false;  // Occluded
            }
        }
    }

    for (int i = 0; i < scene->n_planes; i++) {
        if (ray_plane_intersect(&shadow_ray, &scene->planes[i], &shadow_hit)) {
            if (shadow_hit.t < light_dist) {
                return false;  // Occluded
            }
        }
    }

    return true;  // Light is visible
}

Vec3f phong_shading(const Hit* hit, const Scene* scene, const Vec3f* view_dir, bool compute_shadows) {
    Vec3f color = scene->ambient;  // Start with ambient

    // Add contribution from each light
    for (int i = 0; i < scene->n_lights; i++) {
        const Light* light = &scene->lights[i];

        // Check shadows
        if (compute_shadows && !is_light_visible(scene, &hit->point, &light->position)) {
            continue;  // Light is occluded
        }

        // Compute light direction and distance
        Vec3f light_dir;
        vec3_sub(&light_dir, &light->position, &hit->point);
        float distance = vec3_length(&light_dir);
        vec3_normalize(&light_dir, &light_dir);

        // Diffuse component (Lambertian)
        float n_dot_l = vec3_dot(&hit->normal, &light_dir);
        if (n_dot_l > 0) {
            Vec3f diffuse;
            vec3_mul_comp(&diffuse, &hit->color, &light->color);
            vec3_scale(&diffuse, &diffuse, light->intensity * n_dot_l);
            vec3_add(&color, &color, &diffuse);
        }

        // Specular component (Phong)
        if (hit->specular > 0 && n_dot_l > 0) {
            Vec3f scaled_normal;
            vec3_scale(&scaled_normal, &hit->normal, 2.0f * n_dot_l);
            Vec3f reflect_dir;
            vec3_sub(&reflect_dir, &scaled_normal, &light_dir);
            float r_dot_v = vec3_dot(&reflect_dir, view_dir);

            if (r_dot_v > 0) {
                float spec_intensity = powf(r_dot_v, hit->specular);
                Vec3f specular;
                vec3_scale(&specular, &light->color, light->intensity * spec_intensity);
                vec3_add(&color, &color, &specular);
            }
        }
    }

    Vec3f final_color;
    vec3_clamp(&final_color, &color, 0.0f, 1.0f);
    return final_color;
}

// ============================================================================
// Camera & Ray Generation
// ============================================================================

Ray generate_camera_ray(const Camera* camera, float x, float y, int width, int height) {
    // Compute camera basis vectors
    Vec3f forward;
    vec3_sub(&forward, &camera->look_at, &camera->position);
    vec3_normalize(&forward, &forward);

    Vec3f right;
    vec3_cross(&right, &forward, &camera->up);
    vec3_normalize(&right, &right);

    Vec3f up;
    vec3_cross(&up, &right, &forward);

    // Convert FOV to radians and compute viewport dimensions
    float fov_rad = camera->fov * 3.14159265359f / 180.0f;
    float viewport_height = 2.0f * tanf(fov_rad / 2.0f);
    float viewport_width = viewport_height * camera->aspect;

    // Map pixel coordinates to [-1, 1]
    float u = (x / (width - 1)) * 2.0f - 1.0f;
    float v = (1.0f - y / (height - 1)) * 2.0f - 1.0f;  // Flip y

    // Compute ray direction
    Vec3f scaled_right;
    vec3_scale(&scaled_right, &right, u * viewport_width / 2.0f);

    Vec3f scaled_up;
    vec3_scale(&scaled_up, &up, v * viewport_height / 2.0f);

    Vec3f temp;
    vec3_add(&temp, &forward, &scaled_right);

    Vec3f ray_dir;
    vec3_add(&ray_dir, &temp, &scaled_up);

    Ray ray;
    ray.origin = camera->position;
    vec3_normalize(&ray.direction, &ray_dir);

    return ray;
}

// ============================================================================
// Multithreading Support
// ============================================================================

/**
 * Get number of CPU cores
 */
int get_cpu_cores(void) {
#ifdef _WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    return (int)sysinfo.dwNumberOfProcessors;
#else
    return (int)sysconf(_SC_NPROCESSORS_ONLN);
#endif
}

/**
 * Thread work data structure
 */
typedef struct {
    const Scene* scene;
    const Camera* camera;
    uint8_t* framebuffer;
    int width;
    int height;
    int start_y;  // First row to render
    int end_y;    // Last row to render (exclusive)
    int thread_id;
} ThreadData;

/**
 * Worker function for rendering threads (Windows API)
 */
#ifdef _WIN32
unsigned __stdcall render_thread_worker(void* arg) {
#else
void* render_thread_worker(void* arg) {
#endif
    ThreadData* data = (ThreadData*)arg;

    // Render assigned rows
    for (int y = data->start_y; y < data->end_y; y++) {
        for (int x = 0; x < data->width; x++) {
            Ray ray = generate_camera_ray(data->camera, (float)x, (float)y, data->width, data->height);
            Vec3f color = trace_ray(&ray, data->scene, 0, 0, true);  // Real-time mode (no reflections)

            int idx = (y * data->width + x) * 3;
            data->framebuffer[idx + 0] = float_to_byte(gamma_correct(color.x));
            data->framebuffer[idx + 1] = float_to_byte(gamma_correct(color.y));
            data->framebuffer[idx + 2] = float_to_byte(gamma_correct(color.z));
        }
    }

#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

// ============================================================================
// Rendering
// ============================================================================

Vec3f trace_ray(const Ray* ray, const Scene* scene, int depth, int max_depth, bool compute_shadows) {
    Hit hit;

    if (!ray_scene_intersect(ray, scene, &hit)) {
        // No hit - return background color (sky blue)
        return (Vec3f){0.5f, 0.7f, 1.0f, 0.0f};
    }

    // Compute view direction
    Vec3f view_dir;
    vec3_scale(&view_dir, &ray->direction, -1.0f);
    vec3_normalize(&view_dir, &view_dir);

    // Compute local color
    Vec3f local_color = phong_shading(&hit, scene, &view_dir, compute_shadows);

    // Handle reflections
    if (hit.reflective > 0 && depth < max_depth) {
        // Compute reflection direction
        float n_dot_v = vec3_dot(&hit.normal, &view_dir);
        Vec3f scaled_normal;
        vec3_scale(&scaled_normal, &hit.normal, 2.0f * n_dot_v);
        Vec3f reflect_dir;
        vec3_sub(&reflect_dir, &scaled_normal, &view_dir);

        // Trace reflection ray
        Ray reflect_ray;
        Vec3f scaled_normal_epsilon;
        vec3_scale(&scaled_normal_epsilon, &hit.normal, EPSILON);
        vec3_add(&reflect_ray.origin, &hit.point, &scaled_normal_epsilon);
        vec3_normalize(&reflect_ray.direction, &reflect_dir);

        Vec3f reflect_color = trace_ray(&reflect_ray, scene, depth + 1, max_depth, compute_shadows);

        // Blend local and reflected color
        Vec3f scaled_local;
        vec3_scale(&scaled_local, &local_color, 1.0f - hit.reflective);
        Vec3f scaled_reflect;
        vec3_scale(&scaled_reflect, &reflect_color, hit.reflective);
        vec3_add(&local_color, &scaled_local, &scaled_reflect);
    }

    Vec3f final_color;
    vec3_clamp(&final_color, &local_color, 0.0f, 1.0f);
    return final_color;
}

void render_realtime(
    const Scene* scene,
    const Camera* camera,
    uint8_t* framebuffer,
    int width,
    int height
) {
    // Real-time mode: Primary rays + shadows, no reflections, no AA
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            Ray ray = generate_camera_ray(camera, (float)x, (float)y, width, height);
            Vec3f color = trace_ray(&ray, scene, 0, 0, true);  // depth=0, max_depth=0 (no reflections)

            int idx = (y * width + x) * 3;
            framebuffer[idx + 0] = float_to_byte(gamma_correct(color.x));
            framebuffer[idx + 1] = float_to_byte(gamma_correct(color.y));
            framebuffer[idx + 2] = float_to_byte(gamma_correct(color.z));
        }
    }
}

void render_offline(
    const Scene* scene,
    const Camera* camera,
    const char* output_path,
    const RenderSettings* settings
) {
    // Allocate framebuffer
    uint8_t* framebuffer = (uint8_t*)malloc(settings->width * settings->height * 3);

    // Offline mode: Full quality rendering
    for (int y = 0; y < settings->height; y++) {
        for (int x = 0; x < settings->width; x++) {
            Vec3f color = {0, 0, 0, 0};

            // Supersampling anti-aliasing
            for (int sy = 0; sy < settings->samples_per_pixel; sy++) {
                for (int sx = 0; sx < settings->samples_per_pixel; sx++) {
                    float jitter_x = (float)sx / settings->samples_per_pixel;
                    float jitter_y = (float)sy / settings->samples_per_pixel;

                    Ray ray = generate_camera_ray(
                        camera,
                        (float)x + jitter_x,
                        (float)y + jitter_y,
                        settings->width,
                        settings->height
                    );

                    Vec3f sample_color = trace_ray(&ray, scene, 0, settings->max_bounces, settings->shadows);
                    vec3_add(&color, &color, &sample_color);
                }
            }

            // Average samples
            int total_samples = settings->samples_per_pixel * settings->samples_per_pixel;
            vec3_scale(&color, &color, 1.0f / total_samples);

            int idx = (y * settings->width + x) * 3;
            framebuffer[idx + 0] = float_to_byte(gamma_correct(color.x));
            framebuffer[idx + 1] = float_to_byte(gamma_correct(color.y));
            framebuffer[idx + 2] = float_to_byte(gamma_correct(color.z));
        }

        // Progress indicator
        if (y % 50 == 0) {
            printf("Rendering: %d/%d lines\n", y, settings->height);
        }
    }

    // Save to file
    save_ppm(output_path, framebuffer, settings->width, settings->height);
    free(framebuffer);
}

void render_multithread(
    const Scene* scene,
    const Camera* camera,
    uint8_t* framebuffer,
    int width,
    int height,
    int num_threads
) {
    // Auto-detect thread count if not specified
    if (num_threads <= 0) {
        num_threads = get_cpu_cores();
    }

    // Clamp to reasonable range
    if (num_threads > 16) num_threads = 16;
    if (num_threads < 1) num_threads = 1;

    printf("Using %d threads for rendering...\n", num_threads);

    // Allocate thread data
    ThreadData* thread_data = (ThreadData*)malloc(num_threads * sizeof(ThreadData));

    // Divide image into horizontal strips
    int rows_per_thread = height / num_threads;
    int remaining_rows = height % num_threads;

    int current_y = 0;
    for (int i = 0; i < num_threads; i++) {
        thread_data[i].scene = scene;
        thread_data[i].camera = camera;
        thread_data[i].framebuffer = framebuffer;
        thread_data[i].width = width;
        thread_data[i].height = height;
        thread_data[i].start_y = current_y;
        thread_data[i].end_y = current_y + rows_per_thread + (i < remaining_rows ? 1 : 0);
        thread_data[i].thread_id = i;

        current_y = thread_data[i].end_y;
    }

#ifdef _WIN32
    // Windows threading with _beginthreadex
    HANDLE* threads = (HANDLE*)malloc(num_threads * sizeof(HANDLE));

    // Launch threads
    for (int i = 0; i < num_threads; i++) {
        threads[i] = (HANDLE)_beginthreadex(
            NULL,                           // Security attributes
            0,                              // Stack size (0 = default)
            render_thread_worker,           // Thread function
            &thread_data[i],                // Thread argument
            0,                              // Creation flags
            NULL                            // Thread ID
        );
    }

    // Wait for all threads to complete
    WaitForMultipleObjects(num_threads, threads, TRUE, INFINITE);

    // Close thread handles
    for (int i = 0; i < num_threads; i++) {
        CloseHandle(threads[i]);
    }
#else
    // POSIX threading with pthread
    pthread_t* threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));

    // Launch threads
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, render_thread_worker, &thread_data[i]);
    }

    // Wait for all threads to complete
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
#endif

    // Cleanup
    free(threads);
    free(thread_data);
}

// ============================================================================
// OpenCL GPU Backend
// ============================================================================

#ifdef USE_OPENCL

// Load kernel source from file
char* load_kernel_source(const char* filename, size_t* source_size) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Error: Failed to open kernel file: %s\n", filename);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    *source_size = ftell(fp);
    rewind(fp);

    char* source = (char*)malloc(*source_size + 1);
    if (!source) {
        fclose(fp);
        return NULL;
    }

    size_t read_size = fread(source, 1, *source_size, fp);
    source[read_size] = '\0';
    *source_size = read_size;

    fclose(fp);
    return source;
}

// Render using OpenCL GPU
void render_gpu_opencl(
    const Scene* scene,
    const Camera* camera,
    uint8_t* framebuffer,
    int width,
    int height
) {
    cl_int err;

    // Performance profiling
    clock_t t_init_start, t_buffer_start, t_kernel_start, t_readback_start, t_cleanup_start;
    double time_init, time_buffer, time_kernel, time_readback, time_cleanup;

    t_init_start = clock();

    // 1. Get OpenCL platform
    cl_platform_id platform;
    err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error: Failed to get OpenCL platform (code %d)\n", err);
        fprintf(stderr, "Falling back to CPU scalar...\n");
        render_realtime(scene, camera, framebuffer, width, height);
        return;
    }

    // 2. Get GPU device
    cl_device_id device;
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error: Failed to get GPU device (code %d)\n", err);
        fprintf(stderr, "Falling back to CPU scalar...\n");
        render_realtime(scene, camera, framebuffer, width, height);
        return;
    }

    // Print GPU info
    char device_name[128];
    clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
    printf("Using GPU: %s\n", device_name);

    // 3. Create context
    cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error: Failed to create OpenCL context (code %d)\n", err);
        render_realtime(scene, camera, framebuffer, width, height);
        return;
    }

    // 4. Create command queue
    cl_command_queue queue = clCreateCommandQueue(context, device, 0, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error: Failed to create command queue (code %d)\n", err);
        clReleaseContext(context);
        render_realtime(scene, camera, framebuffer, width, height);
        return;
    }

    // 5. Load and compile kernel
    size_t source_size;
    char* kernel_source = load_kernel_source("src/kernels/ray_tracer.cl", &source_size);
    if (!kernel_source) {
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        render_realtime(scene, camera, framebuffer, width, height);
        return;
    }

    cl_program program = clCreateProgramWithSource(context, 1, (const char**)&kernel_source, &source_size, &err);
    free(kernel_source);

    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error: Failed to create program (code %d)\n", err);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        render_realtime(scene, camera, framebuffer, width, height);
        return;
    }

    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error: Failed to build program (code %d)\n", err);

        // Print build log
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char* build_log = (char*)malloc(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, build_log, NULL);
        fprintf(stderr, "Build log:\n%s\n", build_log);
        free(build_log);

        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        render_realtime(scene, camera, framebuffer, width, height);
        return;
    }

    cl_kernel kernel = clCreateKernel(program, "raytrace_primary", &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error: Failed to create kernel (code %d)\n", err);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        render_realtime(scene, camera, framebuffer, width, height);
        return;
    }

    printf("OpenCL kernel compiled successfully!\n");

    t_buffer_start = clock();
    time_init = (double)(t_buffer_start - t_init_start) / CLOCKS_PER_SEC;

    // 6. Create GPU buffers
    size_t fb_size = width * height * 3;
    cl_mem fb_buf = clCreateBuffer(context, CL_MEM_WRITE_ONLY, fb_size, NULL, &err);

    cl_mem sphere_buf = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                       scene->n_spheres * sizeof(Sphere), scene->spheres, &err);

    cl_mem plane_buf = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                      scene->n_planes * sizeof(Plane), scene->planes, &err);

    cl_mem light_buf = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                      scene->n_lights * sizeof(Light), scene->lights, &err);

    // 7. Set kernel arguments
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &fb_buf);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &sphere_buf);
    clSetKernelArg(kernel, 2, sizeof(int), &scene->n_spheres);
    clSetKernelArg(kernel, 3, sizeof(cl_mem), &plane_buf);
    clSetKernelArg(kernel, 4, sizeof(int), &scene->n_planes);
    clSetKernelArg(kernel, 5, sizeof(cl_mem), &light_buf);
    clSetKernelArg(kernel, 6, sizeof(int), &scene->n_lights);
    clSetKernelArg(kernel, 7, sizeof(Camera), camera);
    clSetKernelArg(kernel, 8, sizeof(Vec3f), &scene->ambient);
    clSetKernelArg(kernel, 9, sizeof(int), &width);
    clSetKernelArg(kernel, 10, sizeof(int), &height);

    t_kernel_start = clock();
    time_buffer = (double)(t_kernel_start - t_buffer_start) / CLOCKS_PER_SEC;

    // 8. Launch kernel
    size_t global_work_size[2] = {(size_t)width, (size_t)height};

    printf("Launching %zu x %zu GPU threads (%zu total)...\n",
           global_work_size[0], global_work_size[1],
           global_work_size[0] * global_work_size[1]);

    // Let OpenCL choose optimal work group size
    err = clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global_work_size, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error: Failed to launch kernel (code %d)\n", err);
    } else {
        clFinish(queue);  // Wait for kernel to complete

        t_readback_start = clock();
        time_kernel = (double)(t_readback_start - t_kernel_start) / CLOCKS_PER_SEC;

        printf("GPU rendering complete!\n");

        // 9. Read back framebuffer
        err = clEnqueueReadBuffer(queue, fb_buf, CL_TRUE, 0, fb_size, framebuffer, 0, NULL, NULL);
        if (err != CL_SUCCESS) {
            fprintf(stderr, "Error: Failed to read framebuffer (code %d)\n", err);
        }

        t_cleanup_start = clock();
        time_readback = (double)(t_cleanup_start - t_readback_start) / CLOCKS_PER_SEC;
    }

    // Print performance breakdown
    printf("\n=== GPU Performance Breakdown ===\n");
    printf("Initialization (platform, context, kernel compile): %.3f ms\n", time_init * 1000);
    printf("Buffer creation + data transfer:                   %.3f ms\n", time_buffer * 1000);
    printf("Kernel execution:                                   %.3f ms\n", time_kernel * 1000);
    printf("Readback (GPU -> CPU):                              %.3f ms\n", time_readback * 1000);
    printf("Total GPU time:                                     %.3f ms\n", (time_init + time_buffer + time_kernel + time_readback) * 1000);
    printf("=================================\n\n");

    // 10. Cleanup
    clock_t t_cleanup_end_start = clock();

    clReleaseMemObject(fb_buf);
    clReleaseMemObject(sphere_buf);
    clReleaseMemObject(plane_buf);
    clReleaseMemObject(light_buf);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    time_cleanup = (double)(clock() - t_cleanup_end_start) / CLOCKS_PER_SEC;
    printf("Cleanup time: %.3f ms\n", time_cleanup * 1000);
}

// ============================================================================
// Persistent GPU Context API (High-Performance)
// ============================================================================

/**
 * GPU Context - Persistent OpenCL state
 */
struct GPUContext {
    // OpenCL resources (persistent across frames)
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;

    // Scene buffers (uploaded once, reused)
    cl_mem sphere_buf;
    cl_mem plane_buf;
    cl_mem light_buf;

    // Scene metadata
    int n_spheres;
    int n_planes;
    int n_lights;
    Vec3f ambient;

    // Framebuffer (created per-resolution)
    cl_mem fb_buf;
    int fb_width;
    int fb_height;
};

/**
 * Initialize persistent GPU context (ONCE at startup)
 */
GPUContext* gpu_init(const Scene* scene) {
    GPUContext* gpu = (GPUContext*)malloc(sizeof(GPUContext));
    if (!gpu) {
        fprintf(stderr, "Error: Failed to allocate GPU context\n");
        return NULL;
    }

    memset(gpu, 0, sizeof(GPUContext));
    cl_int err;

    printf("[GPU] Initializing persistent context...\n");
    clock_t t_start = clock();

    // 1. Get OpenCL platform
    err = clGetPlatformIDs(1, &gpu->platform, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[GPU] Error: Failed to get OpenCL platform (code %d)\n", err);
        free(gpu);
        return NULL;
    }

    // 2. Get GPU device
    err = clGetDeviceIDs(gpu->platform, CL_DEVICE_TYPE_GPU, 1, &gpu->device, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[GPU] Error: Failed to get GPU device (code %d)\n", err);
        free(gpu);
        return NULL;
    }

    // Print GPU info
    char device_name[128];
    clGetDeviceInfo(gpu->device, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
    printf("[GPU] Using device: %s\n", device_name);

    // 3. Create context
    gpu->context = clCreateContext(NULL, 1, &gpu->device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[GPU] Error: Failed to create context (code %d)\n", err);
        free(gpu);
        return NULL;
    }

    // 4. Create command queue
    gpu->queue = clCreateCommandQueue(gpu->context, gpu->device, 0, &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[GPU] Error: Failed to create command queue (code %d)\n", err);
        clReleaseContext(gpu->context);
        free(gpu);
        return NULL;
    }

    // 5. Load and compile kernel (THIS IS THE SLOW PART!)
    printf("[GPU] Compiling kernel from disk...\n");
    size_t source_size;
    char* kernel_source = load_kernel_source("src/kernels/ray_tracer.cl", &source_size);
    if (!kernel_source) {
        clReleaseCommandQueue(gpu->queue);
        clReleaseContext(gpu->context);
        free(gpu);
        return NULL;
    }

    gpu->program = clCreateProgramWithSource(gpu->context, 1, (const char**)&kernel_source, &source_size, &err);
    free(kernel_source);

    if (err != CL_SUCCESS) {
        fprintf(stderr, "[GPU] Error: Failed to create program (code %d)\n", err);
        clReleaseCommandQueue(gpu->queue);
        clReleaseContext(gpu->context);
        free(gpu);
        return NULL;
    }

    err = clBuildProgram(gpu->program, 1, &gpu->device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[GPU] Error: Failed to build program (code %d)\n", err);

        size_t log_size;
        clGetProgramBuildInfo(gpu->program, gpu->device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char* build_log = (char*)malloc(log_size);
        clGetProgramBuildInfo(gpu->program, gpu->device, CL_PROGRAM_BUILD_LOG, log_size, build_log, NULL);
        fprintf(stderr, "[GPU] Build log:\n%s\n", build_log);
        free(build_log);

        clReleaseProgram(gpu->program);
        clReleaseCommandQueue(gpu->queue);
        clReleaseContext(gpu->context);
        free(gpu);
        return NULL;
    }

    gpu->kernel = clCreateKernel(gpu->program, "raytrace_primary", &err);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[GPU] Error: Failed to create kernel (code %d)\n", err);
        clReleaseProgram(gpu->program);
        clReleaseCommandQueue(gpu->queue);
        clReleaseContext(gpu->context);
        free(gpu);
        return NULL;
    }

    printf("[GPU] Kernel compiled successfully!\n");

    // 6. Upload scene geometry to GPU (once!)
    printf("[GPU] Uploading scene geometry to GPU...\n");

    gpu->n_spheres = scene->n_spheres;
    gpu->n_planes = scene->n_planes;
    gpu->n_lights = scene->n_lights;
    gpu->ambient = scene->ambient;

    gpu->sphere_buf = clCreateBuffer(gpu->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                     scene->n_spheres * sizeof(Sphere), scene->spheres, &err);

    gpu->plane_buf = clCreateBuffer(gpu->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                    scene->n_planes * sizeof(Plane), scene->planes, &err);

    gpu->light_buf = clCreateBuffer(gpu->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                    scene->n_lights * sizeof(Light), scene->lights, &err);

    // Framebuffer will be created on first render
    gpu->fb_buf = NULL;
    gpu->fb_width = 0;
    gpu->fb_height = 0;

    double elapsed = (double)(clock() - t_start) / CLOCKS_PER_SEC;
    printf("[GPU] Initialization complete in %.3f ms\n", elapsed * 1000);
    printf("[GPU] Scene: %d spheres, %d planes, %d lights\n",
           gpu->n_spheres, gpu->n_planes, gpu->n_lights);

    return gpu;
}

/**
 * Render single frame (HOT PATH - call every frame)
 */
void gpu_render_frame(
    GPUContext* gpu,
    const Camera* camera,
    uint8_t* framebuffer,
    int width,
    int height
) {
    if (!gpu) {
        fprintf(stderr, "[GPU] Error: NULL GPU context\n");
        return;
    }

    cl_int err;

    // Create/recreate framebuffer if resolution changed
    if (gpu->fb_buf == NULL || gpu->fb_width != width || gpu->fb_height != height) {
        if (gpu->fb_buf) {
            clReleaseMemObject(gpu->fb_buf);
        }

        size_t fb_size = width * height * 3;
        gpu->fb_buf = clCreateBuffer(gpu->context, CL_MEM_WRITE_ONLY, fb_size, NULL, &err);
        gpu->fb_width = width;
        gpu->fb_height = height;

        printf("[GPU] Created framebuffer: %dx%d\n", width, height);
    }

    // Set kernel arguments (camera can change every frame)
    clSetKernelArg(gpu->kernel, 0, sizeof(cl_mem), &gpu->fb_buf);
    clSetKernelArg(gpu->kernel, 1, sizeof(cl_mem), &gpu->sphere_buf);
    clSetKernelArg(gpu->kernel, 2, sizeof(int), &gpu->n_spheres);
    clSetKernelArg(gpu->kernel, 3, sizeof(cl_mem), &gpu->plane_buf);
    clSetKernelArg(gpu->kernel, 4, sizeof(int), &gpu->n_planes);
    clSetKernelArg(gpu->kernel, 5, sizeof(cl_mem), &gpu->light_buf);
    clSetKernelArg(gpu->kernel, 6, sizeof(int), &gpu->n_lights);
    clSetKernelArg(gpu->kernel, 7, sizeof(Camera), camera);
    clSetKernelArg(gpu->kernel, 8, sizeof(Vec3f), &gpu->ambient);
    clSetKernelArg(gpu->kernel, 9, sizeof(int), &width);
    clSetKernelArg(gpu->kernel, 10, sizeof(int), &height);

    // Launch kernel (ONE THREAD PER PIXEL!)
    size_t global_work_size[2] = {(size_t)width, (size_t)height};

    err = clEnqueueNDRangeKernel(gpu->queue, gpu->kernel, 2, NULL, global_work_size, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[GPU] Error: Failed to launch kernel (code %d)\n", err);
        return;
    }

    // Wait for kernel to complete
    clFinish(gpu->queue);

    // Read back framebuffer
    size_t fb_size = width * height * 3;
    err = clEnqueueReadBuffer(gpu->queue, gpu->fb_buf, CL_TRUE, 0, fb_size, framebuffer, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "[GPU] Error: Failed to read framebuffer (code %d)\n", err);
    }
}

/**
 * Update scene geometry (optional - for dynamic scenes)
 */
void gpu_update_scene(GPUContext* gpu, const Scene* scene) {
    if (!gpu) return;

    printf("[GPU] Updating scene geometry...\n");

    // Release old buffers
    clReleaseMemObject(gpu->sphere_buf);
    clReleaseMemObject(gpu->plane_buf);
    clReleaseMemObject(gpu->light_buf);

    // Upload new scene
    cl_int err;
    gpu->n_spheres = scene->n_spheres;
    gpu->n_planes = scene->n_planes;
    gpu->n_lights = scene->n_lights;
    gpu->ambient = scene->ambient;

    gpu->sphere_buf = clCreateBuffer(gpu->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                     scene->n_spheres * sizeof(Sphere), scene->spheres, &err);

    gpu->plane_buf = clCreateBuffer(gpu->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                    scene->n_planes * sizeof(Plane), scene->planes, &err);

    gpu->light_buf = clCreateBuffer(gpu->context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                    scene->n_lights * sizeof(Light), scene->lights, &err);
}

/**
 * Cleanup GPU context (ONCE at shutdown)
 */
void gpu_cleanup(GPUContext* gpu) {
    if (!gpu) return;

    printf("[GPU] Cleaning up persistent context...\n");

    if (gpu->fb_buf) clReleaseMemObject(gpu->fb_buf);
    if (gpu->sphere_buf) clReleaseMemObject(gpu->sphere_buf);
    if (gpu->plane_buf) clReleaseMemObject(gpu->plane_buf);
    if (gpu->light_buf) clReleaseMemObject(gpu->light_buf);
    if (gpu->kernel) clReleaseKernel(gpu->kernel);
    if (gpu->program) clReleaseProgram(gpu->program);
    if (gpu->queue) clReleaseCommandQueue(gpu->queue);
    if (gpu->context) clReleaseContext(gpu->context);

    free(gpu);

    printf("[GPU] Cleanup complete\n");
}

#endif // USE_OPENCL

// ============================================================================
// Unified Rendering API
// ============================================================================

void render_frame(
    const Scene* scene,
    const Camera* camera,
    uint8_t* framebuffer,
    int width,
    int height,
    RenderBackend backend
) {
    // Auto-detect best backend
    if (backend == RENDER_BACKEND_AUTO) {
        // Prefer multithreading if we have multiple cores
        int cores = get_cpu_cores();
        if (cores >= 2) {
            backend = RENDER_BACKEND_CPU_MULTITHREAD;
        } else {
            backend = RENDER_BACKEND_CPU_SCALAR;
        }
    }

    // Dispatch to appropriate backend
    switch (backend) {
        case RENDER_BACKEND_CPU_SCALAR:
            printf("Backend: CPU Scalar\n");
            render_realtime(scene, camera, framebuffer, width, height);
            break;

        case RENDER_BACKEND_CPU_MULTITHREAD:
            printf("Backend: CPU Multithreaded\n");
            render_multithread(scene, camera, framebuffer, width, height, 0);  // 0 = auto-detect cores
            break;

        case RENDER_BACKEND_GPU_OPENCL:
            printf("Backend: GPU OpenCL\n");
#ifdef USE_OPENCL
            render_gpu_opencl(scene, camera, framebuffer, width, height);
#else
            fprintf(stderr, "Error: OpenCL support not compiled! Use -DUSE_OPENCL\n");
            fprintf(stderr, "Falling back to CPU scalar...\n");
            render_realtime(scene, camera, framebuffer, width, height);
#endif
            break;

        default:
            fprintf(stderr, "Error: Unknown backend %d\n", backend);
            render_realtime(scene, camera, framebuffer, width, height);
            break;
    }
}

// ============================================================================
// Scene Creation Helpers
// ============================================================================

Camera create_camera(Vec3f position, Vec3f look_at, Vec3f up, float fov, float aspect) {
    Camera camera;
    camera.position = position;
    camera.look_at = look_at;
    camera.up = up;
    camera.fov = fov;
    camera.aspect = aspect;
    return camera;
}

Sphere create_sphere(Vec3f center, float radius, Vec3f color, float specular, float reflective) {
    Sphere sphere;
    sphere.center = center;
    sphere.radius = radius;
    sphere.color = color;
    sphere.specular = specular;
    sphere.reflective = reflective;
    return sphere;
}

Plane create_plane(Vec3f normal, float distance, Vec3f color, float specular) {
    Plane plane;
    plane.normal = normal;
    plane.distance = distance;
    plane.color = color;
    plane.specular = specular;
    return plane;
}

Light create_light(Vec3f position, Vec3f color, float intensity) {
    Light light;
    light.position = position;
    light.color = color;
    light.intensity = intensity;
    return light;
}

Scene create_scene(void) {
    Scene scene;
    scene.spheres = NULL;
    scene.n_spheres = 0;
    scene.planes = NULL;
    scene.n_planes = 0;
    scene.lights = NULL;
    scene.n_lights = 0;
    scene.ambient = (Vec3f){0.1f, 0.1f, 0.1f, 0.0f};
    return scene;
}

void free_scene(Scene* scene) {
    if (scene->spheres) free(scene->spheres);
    if (scene->planes) free(scene->planes);
    if (scene->lights) free(scene->lights);
}

// ============================================================================
// Utility Functions
// ============================================================================

void save_ppm(const char* filename, const uint8_t* pixels, int width, int height) {
    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "Error: Could not open file %s\n", filename);
        return;
    }

    fprintf(fp, "P6\n%d %d\n255\n", width, height);
    fwrite(pixels, 1, width * height * 3, fp);
    fclose(fp);

    printf("Saved: %s (%dx%d)\n", filename, width, height);
}

uint8_t float_to_byte(float f) {
    int i = (int)(f * 255.0f + 0.5f);
    return (uint8_t)MAX(0, MIN(255, i));
}

float gamma_correct(float linear) {
    // sRGB gamma correction
    if (linear <= 0.0031308f) {
        return 12.92f * linear;
    } else {
        return 1.055f * powf(linear, 1.0f / 2.4f) - 0.055f;
    }
}
