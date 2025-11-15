# Why This Code Was Archived

**Date:** November 10, 2025
**Reason:** Violated FP-first principles - Complete architectural mismatch with project mission

---

## The Problem

This entire engine codebase was written in **traditional imperative C style** with:
- ❌ Mutable state everywhere
- ❌ Imperative loops (`for`, `while`)
- ❌ In-place mutations
- ❌ Side effects mixed with logic
- ❌ **ZERO usage of FP library functions**

## Project Mission

**This is a showcase for the FP-ASM library!**

Every computation should be:
- ✅ Composed from `fp_map`, `fp_fold`, `fp_reduce` functions
- ✅ Immutable data structures
- ✅ Pure functions (no side effects)
- ✅ Functional composition visible

## What's Wrong With Each File

### `ecs.c` (24KB) - Entity Component System
**Violations:**
```c
// MUTABLE ARRAYS
typedef struct {
    Entity entities[MAX_ENTITIES];  // MUTATED!
    size_t entity_count;            // MUTATED!
    // ... more mutable state
} ECSWorld;

// IMPERATIVE LOOPS
for (Entity id = 0; id < world->entity_count; ++id) {
    // MUTATION!
    world->entities[id] = /* ... */;
}
```

**Should be:**
- Immutable entity collections
- Pure transform functions returning new collections
- Use `fp_map` for bulk entity updates

---

### `renderer_modern.c` (41KB) - Renderer
**Violations:**
```c
// SSAO kernel generation - IMPERATIVE!
for (int i = 0; i < sample_count; ++i) {
    kernel[i].x = sinf(phi) * cosf(theta);  // MUTATION!
    kernel[i].y = /* ... */;                 // MUTATION!
}

// NO FP LIBRARY USAGE AT ALL!
```

**Should be:**
- SSAO computation using `fp_reduce_add_f32` (like `demo_fp_ssao.c`)
- Matrix operations using `fp_fold_dotp_f32`
- Immutable render state

---

### `shaders_embedded.c` (23KB) - Shader Strings
**Status:** This is just data (string constants), so it's actually fine.

**But:** Shader *usage* in renderer was imperative.

---

### `gl_extensions.c` (4.6KB) - OpenGL Extension Loading
**Violations:**
```c
// GLOBAL MUTABLE STATE
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = NULL;  // MUTATION!
PFNGLBINDVERTEXARRAYPROC glBindVertexArray = NULL;  // MUTATION!
// ... etc

// IMPERATIVE LOADING
glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)wglGetProcAddress("glGenVertexArrays");
if (!glGenVertexArrays) {
    // ERROR HANDLING WITH SIDE EFFECTS
    fprintf(stderr, "Failed to load...\n");
}
```

**Should be:**
- Immutable function pointer structure returned from loader
- Pure validation functions
- Errors as return values, not fprintf

---

## The Correct Example

**`demo_fp_ssao.c` (375 lines)** shows the RIGHT approach:

```c
// IMMUTABLE CONFIG
typedef struct {
    const int sample_count;
    const float radius;
    const int width;
    const int height;
} SSAOConfig;

// PURE FUNCTION - Uses FP library!
float ssao_compute_pixel_fp(
    const float* depth_buffer,  // const!
    const float* kernel_x,       // const!
    /* ... */
) {
    float occlusion_flags[MAX_SAMPLES];
    // ... generate flags ...

    // FP LIBRARY USAGE - This is the point!
    float total = fp_reduce_add_f32(occlusion_flags, config.sample_count);

    return 1.0f - (total / (float)config.sample_count);
}
```

**This is how ALL code should be written!**

---

## Statistics

| File | Lines | FP Library Calls | Imperative Loops | Verdict |
|------|-------|------------------|------------------|---------|
| `ecs.c` | ~600 | **0** | ~15 | ❌ FAIL |
| `renderer_modern.c` | ~1000 | **0** | ~30 | ❌ FAIL |
| `gl_extensions.c` | ~120 | **0** | ~5 | ❌ FAIL |
| `shaders_embedded.c` | ~400 | N/A (data) | 0 | ⚠️ OK (just data) |
| **Total** | **~2120** | **0** | **~50** | **❌ COMPLETE FAILURE** |

Compare to:
| File | Lines | FP Library Calls | Imperative Loops | Verdict |
|------|-------|------------------|------------------|---------|
| `demo_fp_ssao.c` | 375 | **65,536+** (runtime) | 0 (hot path) | ✅ **SUCCESS** |

---

## Lessons Learned

1. **This is an FP library project, not a game engine project**
2. **Every algorithm must showcase FP library functions**
3. **Immutability is not optional - it's the entire point**
4. **Pure functions everywhere**
5. **No imperative loops in computational hot paths**

---

## What To Do Instead

See:
- `demo_fp_ssao.c` - FP-first SSAO (CORRECT)
- `FP_SSAO_DESIGN.md` - Design doc for FP-first graphics
- Future: `src/engine/` will contain FP-first architecture

---

**This code is preserved as a historical note of what NOT to do.**

The refactored FP-first engine will be in `src/engine/`.
