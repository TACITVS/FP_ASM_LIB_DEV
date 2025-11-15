# Hybrid CPU+GPU Ray Tracer - Implementation Plan

## Overview

Multi-backend ray tracer demonstrating modern parallel computing:
- **CPU Scalar**: Baseline (16.67 FPS @ 400x300) ✅ DONE
- **CPU SIMD**: AVX2 4-wide optimization (target: 50+ FPS @ 400x300)
- **GPU OpenCL**: Massively parallel compute (target: 60+ FPS @ 1920x1080)

## Architecture

```
┌────────────────────────────────────────────────┐
│         Unified Ray Tracer API                 │
├────────────────────────────────────────────────┤
│  render_frame(scene, camera, fb, backend)      │
└───────────┬────────────────────────────────────┘
            │
    ┌───────┴───────┬───────────────┬─────────┐
    │               │               │         │
┌───▼────┐   ┌──────▼──────┐   ┌───▼──────┐ │
│ SCALAR │   │ SIMD (AVX2) │   │  OpenCL  │ │
│  CPU   │   │    CPU      │   │   GPU    │ │
│        │   │             │   │          │ │
│ 16 FPS │   │  50+ FPS    │   │ 100+ FPS │ │
└────────┘   └─────────────┘   └──────────┘ │
                                             │
                                   ┌─────────▼──┐
                                   │ CUDA (fut) │
                                   └────────────┘
```

## Performance Targets

### Your i7-4600M + GT 730M System

| Backend | Resolution | Target FPS | Realistic FPS | Status |
|---------|-----------|------------|---------------|--------|
| CPU Scalar | 400x300 | 15 | **16.67** ✅ | Done |
| CPU Scalar | 640x480 | 5 | 6.5 | Done |
| CPU SIMD | 640x480 | 30 | 20-25 | Pending |
| CPU SIMD | 800x600 | 15 | 10-15 | Pending |
| GPU OpenCL | 1920x1080 | 60 | 30-60 | Pending |
| GPU OpenCL | 640x480 | 120 | 100+ | Pending |

## Implementation Phases

### Phase 1: AVX2 CPU Optimization (30-60 min)

**Goal**: Process 4 rays simultaneously with SIMD

**Key optimization**: Ray-sphere intersection batch
```c
// Instead of:
for (int i = 0; i < 4; i++) {
    intersect_ray_sphere(&rays[i], sphere, &hits[i]);
}

// Do this (SIMD):
intersect_4_rays_sphere_avx2(rays, sphere, hits);
```

**Expected speedup**: 2.5-3.5x (not full 4x due to memory bandwidth)

**Implementation files**:
- `fp_ray_tracer_simd.c` - AVX2 intrinsics
- Modify existing functions to batch process 4 rays

### Phase 2: Backend Abstraction (15 min)

**Add backend enum**:
```c
typedef enum {
    RENDER_BACKEND_CPU_SCALAR,
    RENDER_BACKEND_CPU_SIMD,
    RENDER_BACKEND_GPU_OPENCL,
    RENDER_BACKEND_GPU_CUDA  // Future
} RenderBackend;
```

**Unified API**:
```c
void render_frame(
    const Scene* scene,
    const Camera* camera,
    uint8_t* framebuffer,
    int width,
    int height,
    RenderBackend backend
);
```

### Phase 3: OpenCL GPU Backend (2-3 hours)

**Goal**: Leverage GT 730M's 384 CUDA cores

**OpenCL kernel** (`ray_tracer.cl`):
```c
__kernel void raytrace_primary(
    __global uchar* framebuffer,
    __global Sphere* spheres,
    __global Light* lights,
    __global Plane* planes,
    Camera camera,
    int width,
    int height,
    int n_spheres,
    int n_lights,
    int n_planes
) {
    int x = get_global_id(0);
    int y = get_global_id(1);

    if (x >= width || y >= height) return;

    // Each thread = 1 pixel
    Ray ray = generate_camera_ray(camera, x, y, width, height);
    vec3 color = trace_ray(ray, spheres, n_spheres,
                           lights, n_lights,
                           planes, n_planes);

    int idx = (y * width + x) * 3;
    framebuffer[idx + 0] = float_to_byte(color.x);
    framebuffer[idx + 1] = float_to_byte(color.y);
    framebuffer[idx + 2] = float_to_byte(color.z);
}
```

**Host code** (`fp_ray_tracer_opencl.c`):
```c
void render_opencl(Scene* scene, Camera* camera,
                   uint8_t* fb, int w, int h) {
    // 1. Initialize OpenCL
    cl_context ctx = create_opencl_context();
    cl_command_queue queue = create_command_queue(ctx);

    // 2. Load and compile kernel
    cl_program program = build_kernel("ray_tracer.cl", ctx);
    cl_kernel kernel = clCreateKernel(program, "raytrace_primary");

    // 3. Create GPU buffers
    cl_mem fb_buf = clCreateBuffer(ctx, w*h*3);
    cl_mem sphere_buf = clCreateBuffer(ctx, scene->spheres);
    cl_mem light_buf = clCreateBuffer(ctx, scene->lights);

    // 4. Set kernel arguments
    clSetKernelArg(kernel, 0, fb_buf);
    clSetKernelArg(kernel, 1, sphere_buf);
    // ...

    // 5. Launch kernel (w*h threads!)
    size_t global_work_size[] = {w, h};
    clEnqueueNDRangeKernel(queue, kernel, 2, global_work_size);

    // 6. Read back results
    clEnqueueReadBuffer(queue, fb_buf, fb, w*h*3);
}
```

### Phase 4: Benchmark Suite (30 min)

**Comprehensive comparison**:
```c
void benchmark_all_backends(void) {
    Scene scene = create_test_scene();
    Camera camera = create_test_camera();

    struct {
        RenderBackend backend;
        int width, height;
    } tests[] = {
        {BACKEND_CPU_SCALAR, 400, 300},
        {BACKEND_CPU_SCALAR, 640, 480},
        {BACKEND_CPU_SIMD, 640, 480},
        {BACKEND_CPU_SIMD, 1920, 1080},
        {BACKEND_GPU_OPENCL, 1920, 1080},
        {BACKEND_GPU_OPENCL, 3840, 2160}  // 4K!
    };

    for (each test) {
        run_benchmark(test);
        print_results();
    }
}
```

## Technical Details

### AVX2 SIMD Strategy

**Data layout** (Structure of Arrays):
```c
typedef struct {
    __m256 origin_x;  // 8 floats (but we use 4)
    __m256 origin_y;
    __m256 origin_z;
    __m256 dir_x;
    __m256 dir_y;
    __m256 dir_z;
} Ray4_AVX2;
```

**Ray-sphere intersection** (4 rays at once):
```c
void intersect_4_rays_sphere_avx2(
    const Ray4_AVX2* rays,
    const Sphere* sphere,
    Hit4_AVX2* hits
) {
    // Load sphere center
    __m256 cx = _mm256_set1_ps(sphere->center.x);
    __m256 cy = _mm256_set1_ps(sphere->center.y);
    __m256 cz = _mm256_set1_ps(sphere->center.z);

    // Compute origin - center (4 rays)
    __m256 ocx = _mm256_sub_ps(rays->origin_x, cx);
    __m256 ocy = _mm256_sub_ps(rays->origin_y, cy);
    __m256 ocz = _mm256_sub_ps(rays->origin_z, cz);

    // Compute a = dot(dir, dir) for all 4 rays
    __m256 a = _mm256_fmadd_ps(rays->dir_x, rays->dir_x,
               _mm256_fmadd_ps(rays->dir_y, rays->dir_y,
               _mm256_mul_ps(rays->dir_z, rays->dir_z)));

    // Compute b = 2 * dot(oc, dir) for all 4 rays
    __m256 b = _mm256_fmadd_ps(ocx, rays->dir_x,
               _mm256_fmadd_ps(ocy, rays->dir_y,
               _mm256_mul_ps(ocz, rays->dir_z)));
    b = _mm256_mul_ps(b, _mm256_set1_ps(2.0f));

    // Compute c = dot(oc, oc) - r² for all 4 rays
    __m256 r2 = _mm256_set1_ps(sphere->radius * sphere->radius);
    __m256 c = _mm256_fmadd_ps(ocx, ocx,
               _mm256_fmadd_ps(ocy, ocy,
               _mm256_mul_ps(ocz, ocz)));
    c = _mm256_sub_ps(c, r2);

    // Discriminant = b² - 4ac
    __m256 disc = _mm256_sub_ps(
        _mm256_mul_ps(b, b),
        _mm256_mul_ps(_mm256_set1_ps(4.0f), _mm256_mul_ps(a, c))
    );

    // Check if disc >= 0 (4 comparisons at once!)
    __m256 valid = _mm256_cmp_ps(disc, _mm256_setzero_ps(), _CMP_GE_OQ);

    // Compute t = (-b - sqrt(disc)) / 2a
    __m256 sqrt_disc = _mm256_sqrt_ps(disc);
    __m256 t = _mm256_div_ps(
        _mm256_sub_ps(_mm256_mul_ps(b, _mm256_set1_ps(-1.0f)), sqrt_disc),
        _mm256_mul_ps(_mm256_set1_ps(2.0f), a)
    );

    // Mask out invalid results
    t = _mm256_and_ps(t, valid);

    // Store results
    _mm256_store_ps(hits->t, t);
}
```

### OpenCL Memory Management

**Minimize CPU↔GPU transfers**:
```c
// Upload once at start
upload_scene_to_gpu(scene);

// Render loop
for (frame in animation) {
    update_camera_only(camera);  // Small transfer
    render_gpu(scene_gpu, camera, fb);
    display(fb);
}
```

### GT 730M Optimization

**Your GPU specs**:
- 384 CUDA cores (6 SMs × 64 cores)
- 64-bit memory bus (narrow!)
- 1.8 GHz GDDR5

**Optimization strategies**:
1. **Coalesce memory access**: Ensure threads access contiguous memory
2. **Minimize divergence**: All threads in warp take same path
3. **Reduce register pressure**: Keep local variables minimal
4. **Cache-friendly**: Reuse sphere/light data across threads

**Work group sizing**:
```c
// Local work size (per group)
size_t local_work_size[] = {16, 16};  // 256 threads per group

// Global work size
size_t global_work_size[] = {
    round_up(width, 16),
    round_up(height, 16)
};
```

## File Structure

```
src/algorithms/
  fp_ray_tracer.c              # Scalar baseline (DONE)
  fp_ray_tracer_simd.c         # AVX2 optimization (TODO)
  fp_ray_tracer_opencl.c       # OpenCL host code (TODO)

src/kernels/
  ray_tracer.cl                # OpenCL kernel (TODO)

include/
  fp_ray_tracer.h              # Updated with backend enum

demos/
  demo_ray_tracer_benchmark.c  # All backends comparison
```

## Expected Benchmark Results

### Baseline (Current)
```
Backend: CPU Scalar
Resolution: 400x300
Time: 0.060s (16.67 FPS)
Rays traced: 120,000
Rays/sec: 2.0M
```

### After AVX2
```
Backend: CPU SIMD (AVX2)
Resolution: 640x480
Time: 0.050s (20 FPS)
Rays traced: 307,200
Rays/sec: 6.1M
Speedup vs Scalar: 3.0x
```

### After OpenCL
```
Backend: GPU OpenCL (GT 730M)
Resolution: 1920x1080
Time: 0.030s (33 FPS)
Rays traced: 2,073,600
Rays/sec: 69.1M
Speedup vs Scalar: 34.5x
Speedup vs SIMD: 11.3x
```

## Next Steps

1. ✅ Implement AVX2 ray batching (fp_ray_tracer_simd.c)
2. ✅ Add backend enum and switcher
3. ✅ Implement OpenCL kernel (ray_tracer.cl)
4. ✅ Add OpenCL host code (fp_ray_tracer_opencl.c)
5. ✅ Create comprehensive benchmark
6. ✅ Document in ALGORITHMS_SHOWCASE.md

## Success Criteria

- [ ] AVX2: 20+ FPS @ 640x480
- [ ] OpenCL: 30+ FPS @ 1920x1080
- [ ] GPU speedup: 30-50x vs CPU scalar
- [ ] Code compiles on Windows with MinGW
- [ ] Demonstrates modern parallel programming techniques
