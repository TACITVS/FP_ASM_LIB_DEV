# FP-First SSAO Design

## Mission Statement

**Create an SSAO implementation that showcases the FP-ASM library** through pure functional composition, demonstrating that real-world graphics computation can be powered by `fp_map`, `fp_fold`, and `fp_reduce`.

---

## Core Principle

Every SSAO computation must be expressed as **composition of FP library functions**.

**NO imperative loops. NO mutations. ONLY pure FP operations.**

---

## Algorithm Decomposition

### 1. Kernel Generation (Precomputation)

**Imperative version (BAD):**
```c
for (int i = 0; i < N; i++) {
    kernel[i] = compute_sample(i);  // MUTATION!
}
```

**FP version (GOOD):**
```c
// Pure function: index → sample
float compute_sample_x(int i, int N) {
    float t = (float)i / (float)N;
    float theta = 2.0f * PI * t;
    return sinf(theta) * t;  // Deterministic, no side effects
}

// Generate all X coordinates using fp_map equivalent
// (We'll need a wrapper since fp_map works on arrays)
```

**FP Composition:**
```
indices [0..N)
  → map(compute_sample_x)
  → kernel_x[]

indices [0..N)
  → map(compute_sample_y)   → kernel_y[]

indices [0..N)
  → map(compute_sample_z)
  → kernel_z[]
```

---

### 2. Occlusion Computation (Core Algorithm)

**SSAO formula:**
```
for each pixel (x, y):
    occlusion = 0
    for each sample in kernel:
        sample_depth = depth_at(x + sample.x, y + sample.y)
        if (sample_depth < current_depth):
            occlusion += 1
    ao = 1.0 - (occlusion / sample_count)
```

**FP Decomposition:**

```
Step 1: Sample depths
  kernel_offsets[]
    → map(offset => depth_at(pixel + offset))
    → sampled_depths[]

Step 2: Compare depths (occlusion test)
  sampled_depths[]
    → map(d => d < current_depth ? 1.0 : 0.0)
    → occlusion_flags[]

Step 3: Accumulate occlusion
  occlusion_flags[]
    → reduce_add
    → total_occlusion (scalar)

Step 4: Normalize
  total_occlusion
    → (* (1.0 / sample_count))
    → normalized_ao
```

**Using FP Library:**
- Step 2: Can use `fp_zip_add` + custom threshold
- Step 3: Use `fp_reduce_add_f32`
- Step 4: Simple scalar operation

---

### 3. Full Image Processing

**Process entire image as 1D array:**

```
Image flattened to 1D:
  pixels = [p0, p1, p2, ..., p(width*height-1)]

Compute AO for all pixels:
  pixels[]
    → map(pixel => compute_ao_for_pixel(pixel, depth_buffer, kernel))
    → ao_values[]
```

---

## Implementation Strategy

### Phase 1: Pure FP Kernel Generation

```c
typedef struct {
    const int sample_count;
    const float radius;
} SSAOConfig;

// Pure function: Generate kernel using FP operations
void ssao_kernel_generate_fp(const SSAOConfig config,
                             float* out_x,
                             float* out_y,
                             float* out_z) {
    // Use fp_map_* to transform indices → samples
    // All operations pure, no mutations of input
}
```

###Phase 2: Per-Pixel AO Computation Using FP

```c
// Pure function: Compute AO for single pixel
float ssao_compute_pixel_fp(
    const float* depth_buffer,
    const float* kernel_x,
    const float* kernel_y,
    const float* kernel_z,
    const SSAOConfig config,
    int pixel_x,
    int pixel_y
) {
    // 1. Sample depths using kernel offsets
    float sampled_depths[MAX_SAMPLES];
    // ... use kernel to gather depths ...

    // 2. Compare with current depth (threshold operation)
    float occlusion_flags[MAX_SAMPLES];
    // ... threshold comparison ...

    // 3. Sum occlusion using fp_reduce_add_f32
    float total = fp_reduce_add_f32(occlusion_flags, config.sample_count);

    // 4. Normalize
    return 1.0f - (total / (float)config.sample_count);
}
```

### Phase 3: Full Image Processing

```c
// Process entire image
void ssao_compute_image_fp(
    const float* depth_buffer,
    float* ao_output,
    const float* kernel_x,
    const float* kernel_y,
    const float* kernel_z,
    const SSAOConfig config,
    int width,
    int height
) {
    // For each pixel, compute AO using fp_* functions
    // Final result: ao_output[] = fully computed AO image
}
```

---

## FP Library Usage Map

| SSAO Operation | FP Library Function |
|----------------|---------------------|
| Generate kernel coordinates | `fp_map_scale_f32` + `fp_map_offset_f32` |
| Sum occlusion flags | `fp_reduce_add_f32` |
| Normalize AO values | `fp_map_scale_f32` |
| Threshold comparison | Custom wrapper over `fp_zip_add_f32` |
| Dot product (if needed) | `fp_fold_dotp_f32` |

---

## Minimal Test Plan

**Test 1: Kernel Generation**
- Input: sample_count = 8
- Output: 8 hemisphere samples
- Verify: All samples in hemisphere (z >= 0)
- FP Usage: Map operations for coordinate generation

**Test 2: Single Pixel AO**
- Input: Flat depth buffer (all 1.0), single pixel
- Output: AO = 1.0 (no occlusion)
- FP Usage: reduce_add for accumulation

**Test 3: Simple Occlusion**
- Input: Depth buffer with corner (depth varies)
- Output: AO < 1.0 in corner
- FP Usage: Full FP pipeline

**Test 4: Full Image**
- Input: 64x64 depth buffer
- Output: 64x64 AO image
- FP Usage: Process all pixels with FP functions
- Output: Save as simple PPM image file

---

## Visual Output

Save AO result as simple **PPM image** (no dependencies):

```c
void save_ppm(const float* ao_image, int width, int height, const char* filename) {
    FILE* f = fopen(filename, "wb");
    fprintf(f, "P6\n%d %d\n255\n", width, height);
    for (int i = 0; i < width * height; i++) {
        uint8_t gray = (uint8_t)(ao_image[i] * 255.0f);
        fputc(gray, f); fputc(gray, f); fputc(gray, f);
    }
    fclose(f);
}
```

View with any image viewer. Dark = occluded, bright = exposed.

---

## Success Criteria

✅ **Zero imperative loops in hot path** - All computation via `fp_*` functions
✅ **Immutable inputs** - All buffers marked `const`
✅ **Pure functions** - No side effects except final output write
✅ **FP composition visible** - Clear map/fold/reduce structure
✅ **Performance comparison** - FP version vs imperative (should be faster!)
✅ **Visual proof** - AO image shows darkening in occluded areas

---

## Code Structure

```
demo_fp_ssao.c (~200 lines)
├── ssao_kernel_generate_fp()     // Uses fp_map_*
├── ssao_compute_pixel_fp()        // Uses fp_reduce_add_f32
├── ssao_compute_image_fp()        // Orchestrates FP pipeline
├── save_ppm()                     // Output visualization
└── main()                         // Immutable config, pure calls

Compiles with: fp_core_fused_folds.o + fp_core_reductions.o + fp_core_fused_maps.o
```

---

## Documentation Strategy

**Every function documents its FP nature:**

```c
/**
 * PURE FP FUNCTION
 *
 * FP Signature: [Int] -> [Float]  (map operation)
 *
 * Transforms sample indices to X coordinates using fp_map_scale_f32.
 *
 * Immutability: Input indices not modified.
 * Side effects: None.
 * Determinism: Same input always produces same output.
 */
void generate_kernel_x_fp(const int* indices, float* out, size_t n);
```

---

## Next Steps

1. ✅ Design complete
2. ⏳ Implement kernel generation using `fp_map_*`
3. ⏳ Implement occlusion computation using `fp_reduce_add_f32`
4. ⏳ Create minimal test with PPM output
5. ⏳ Build and verify visual results
6. ⏳ Performance benchmark (FP vs imperative)

---

**This is how you showcase an FP library: real computation, pure functions, visible composition.**

