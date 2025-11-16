# Phase 2: SSAO Refactor Plan

## What Happened

During Phase 2 implementation, I mistakenly added SSAO code directly to `demo_engine_mvp.c` (~400 lines), violating the **modular architecture** principle. This was caught and corrected.

## What Was Reverted

All SSAO code was removed from `demo_engine_mvp.c`:
- ❌ SSAO global variables (FBOs, textures, shader programs)
- ❌ SSAO shader strings (~300 lines of GLSL)
- ❌ SSAO setup functions (setup_fullscreen_quad, setup_scene_fbo, etc.)
- ❌ Calls to SSAO setup in `setup_opengl()`

**Result:** Demo is back to clean state and compiles successfully.

## Correct Architecture

The library **already has SSAO infrastructure planned**:

```
include/renderer_modern.h:
  - RendererConfig.enable_ssao       ✅ Already defined
  - RendererConfig.ssao_samples      ✅ Already defined
  - RendererConfig.ssao_radius       ✅ Already defined
  - Shader.u_ssao_texture            ✅ Already defined

src/engine/renderer_modern.c:
  - SSAO implementation               ❌ NOT YET IMPLEMENTED

src/engine/shaders_embedded.c:
  - SSAO shader strings               ❌ NOT YET ADDED
```

## Implementation Plan (Next Session)

### Step 1: Add SSAO Shaders to Library

**File:** `src/engine/shaders_embedded.c`

Add three new shader strings:
1. `ssao_vertex_shader` - Fullscreen quad vertex shader
2. `ssao_fragment_shader` - Ultra-Lite SSAO (8-sample depth-based AO)
3. `ssao_composite_fragment_shader` - Apply AO to final image

**Estimated:** ~150 lines

### Step 2: Implement SSAO in Renderer

**File:** `src/engine/renderer_modern.c`

Add functions:
```c
// Initialize SSAO system
void renderer_init_ssao(Renderer* r, int width, int height);

// Render SSAO pass (if enabled in config)
void renderer_render_ssao_pass(Renderer* r, Framebuffer* depth_fb);

// Cleanup SSAO resources
void renderer_cleanup_ssao(Renderer* r);
```

**Estimated:** ~200 lines

### Step 3: Update Renderer Header

**File:** `include/renderer_modern.h`

Add to Renderer struct:
```c
typedef struct Renderer {
    // ... existing fields ...

    // SSAO resources
    Framebuffer* ssao_fbo;
    Shader* ssao_shader;
    Shader* ssao_composite_shader;
    GLuint quad_vao;
} Renderer;
```

Add public API functions (from Step 2).

**Estimated:** ~30 lines

### Step 4: Update Demo (Minimal Changes)

**File:** `demo_engine_mvp.c`

Demo should remain simple - just call library:
```c
// In setup:
renderer_init_ssao(renderer, WINDOW_WIDTH, WINDOW_HEIGHT);

// In render loop (library handles enable_ssao flag internally):
renderer_render(renderer, world);  // SSAO applied automatically

// In cleanup:
renderer_cleanup_ssao(renderer);
```

**Estimated:** ~5 lines total

## Key Principles Maintained

✅ **Modularity:** SSAO is entirely in the library
✅ **Reusability:** Any project can use SSAO by linking the library
✅ **FP-First:** SSAO shaders are pure transformations
✅ **Independence:** SSAO toggles via `RenderConfig.enable_ssao`
✅ **Demo as Consumer:** Demo just calls library functions

## SSAO Implementation Details (Preserved from Earlier Work)

### Algorithm: Ultra-Lite SSAO
- **8-sample hemisphere** around each fragment
- **Depth-only** (no G-buffer needed for MVP)
- **Screen-space normals** estimated from depth gradients
- **Single-pass** (no blur for simplicity)

### Shaders to Add:
1. **Fullscreen Quad Vertex Shader** (~10 lines)
2. **SSAO Fragment Shader** (~80 lines):
   - Reconstruct view-space position from depth
   - Generate hemisphere samples
   - Test occlusion
   - Output AO factor
3. **Composite Fragment Shader** (~15 lines):
   - Multiply scene color by AO factor

### FBO Setup:
- **Scene FBO:** Color + Depth textures
- **SSAO FBO:** Single-channel AO texture

### Rendering Pipeline:
```
Pass 1: Render scene → scene_fbo (color + depth)
Pass 2: Compute SSAO from depth → ssao_fbo (if enable_ssao)
Pass 3: Composite to screen (apply AO to color)
```

### Integration with Quality System:
```c
// In demo_engine_mvp.c keyboard handler:
else if (wParam == 'O') {
    quality.enable_ssao = !quality.enable_ssao;
    // Renderer automatically respects this flag
    printf("SSAO: %s\n", quality.enable_ssao ? "ON" : "OFF");
}
```

## Next Session Checklist

- [ ] Read this document
- [ ] Add SSAO shaders to `shaders_embedded.c`
- [ ] Implement SSAO functions in `renderer_modern.c`
- [ ] Update `renderer_modern.h` with SSAO API
- [ ] Update demo to call library SSAO functions
- [ ] Build and test SSAO toggle with 'O' key
- [ ] Verify visual quality improvement
- [ ] Confirm FPS impact (~5ms for 8-sample SSAO)

## Performance Expectations

| Quality | SSAO Samples | Frame Time Impact | Target FPS |
|---------|--------------|-------------------|------------|
| Low     | 0 (OFF)      | 0ms              | 60+        |
| Medium  | 16           | ~3ms             | 45-60      |
| High    | 32           | ~5ms             | 30-45      |
| Ultra   | 64           | ~8ms             | 30+        |

## Success Criteria

1. ✅ Demo compiles and runs without SSAO
2. ✅ SSAO code entirely in library
3. ⏳ SSAO toggles with 'O' key
4. ⏳ Visual improvement: dark areas in corners/crevices
5. ⏳ Performance acceptable: <10ms overhead

## What's Working Right Now

✅ **Phase 1 Complete:**
- Modular quality settings structure
- Quality presets (F1-F4)
- Individual feature toggles (O/P/H/B/M/G)
- All keyboard controls working
- MSAA toggle functional

✅ **Architecture Clean:**
- Demo has no SSAO knowledge
- Library has SSAO hooks ready
- Quality system in place

## Repository Status

```
demo_engine_mvp.c        - Clean, no SSAO code ✅
include/renderer_modern.h - SSAO planned but not implemented
src/engine/renderer_modern.c - SSAO not implemented
src/engine/shaders_embedded.c - No SSAO shaders yet
```

---

**Next Step:** Implement SSAO in the library following this plan.
**Philosophy:** "Slow and steady wins the race" + "Modularity is foundational"
