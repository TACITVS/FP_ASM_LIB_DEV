# SSAO Implementation - Required Changes

## Summary
Converting from **Forward Rendering** to **Deferred Rendering + SSAO** requires significant architectural changes to the demo_engine_mvp.c file.

## Code Size Estimate
- **Current demo_engine_mvp.c**: ~1,100 lines
- **With SSAO additions**: ~2,200 lines (doubles in size)
- **New code**: ~1,100 lines of new functionality

## Required Additions

### 1. New Global Variables (~50 lines)
```c
// G-buffer framebuffer and textures
GLuint gBuffer;
GLuint gPosition, gNormal, gAlbedoSpec;
GLuint gDepth;

// SSAO framebuffers and textures
GLuint ssaoFBO, ssaoBlurFBO;
GLuint ssaoColorBuffer, ssaoColorBufferBlur;

// SSAO kernel and noise
Vec3 ssaoKernel[64];
GLuint noiseTexture;

// New shader programs
GLuint geometryShader;
GLuint ssaoShader;
GLuint ssaoBlurShader;
GLuint lightingShader;

// SSAO parameters (for keyboard control)
float ssaoRadius = 0.5f;
float ssaoBias = 0.025f;
int ssaoKernelSize = 64;
BOOL ssaoEnabled = TRUE;

// Fullscreen quad for screen-space passes
GLuint quadVAO;
```

### 2. New Shader Definitions (~500 lines)
```c
// Geometry pass vertex shader (~30 lines)
const char* geometry_vertex_shader = "...";

// Geometry pass fragment shader (~40 lines)
const char* geometry_fragment_shader = "...";

// SSAO shader (~100 lines)
const char* ssao_shader = "...";

// SSAO blur shader (~40 lines)
const char* ssao_blur_shader = "...";

// Lighting pass vertex shader (~30 lines)
const char* lighting_vertex_shader = "...";

// Lighting pass fragment shader (~150 lines)
const char* lighting_fragment_shader = "...";

// Fullscreen quad vertex shader (~20 lines)
const char* quad_vertex_shader = "...";

// Fullscreen quad fragment shader (~20 lines)
const char* quad_fragment_shader = "...";
```

### 3. New Initialization Functions (~300 lines)

```c
// Create all framebuffers and textures
void setup_gbuffer(void) {
    // Create G-buffer FBO
    // Create position texture (RGB32F)
    // Create normal texture (RGB16F)
    // Create albedo+spec texture (RGBA8)
    // Attach depth renderbuffer
    // ~80 lines
}

void setup_ssao_framebuffers(void) {
    // Create SSAO FBO
    // Create SSAO blur FBO
    // Create textures
    // ~40 lines
}

void generate_ssao_kernel(void) {
    // Generate 64 random hemisphere samples
    // Distribute with lerp to concentrate near origin
    // ~30 lines
}

void generate_ssao_noise(void) {
    // Create 4x4 noise texture
    // Random tangent-space rotation vectors
    // ~30 lines
}

void setup_fullscreen_quad(void) {
    // Create VAO/VBO for fullscreen quad
    // ~20 lines
}

void setup_ssao_shaders(void) {
    // Create geometry shader
    // Create SSAO shader
    // Create blur shader
    // Create lighting shader
    // Get all uniform locations
    // ~100 lines
}
```

### 4. Modified Rendering Pipeline (~250 lines)

**OLD render_world():**
```c
void render_world() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // Bind PBR shader
    // Set uniforms
    // Draw all meshes
    // Done - ~90 lines
}
```

**NEW render_world():**
```c
void render_world() {
    // PASS 1: Geometry Pass
    // Bind G-buffer FBO
    // Clear attachments
    // For each mesh:
    //   - Bind geometry shader
    //   - Set model/view/projection
    //   - Set material uniforms
    //   - Draw mesh
    // (~50 lines)

    // PASS 2: SSAO Pass
    // Bind SSAO FBO
    // Bind SSAO shader
    // Bind G-buffer textures
    // Set SSAO kernel uniforms
    // Set noise texture
    // Draw fullscreen quad
    // (~40 lines)

    // PASS 3: SSAO Blur Pass
    // Bind SSAO blur FBO
    // Bind blur shader
    // Bind SSAO texture
    // Draw fullscreen quad
    // (~30 lines)

    // PASS 4: Lighting Pass
    // Bind default framebuffer
    // Bind lighting shader
    // Bind G-buffer textures
    // Bind SSAO blur texture
    // Set light uniforms
    // Draw fullscreen quad
    // (~50 lines)

    // PASS 5: Post-process (tone mapping)
    // Already handled by lighting shader
    // (~0 lines - integrated)

    // Total: ~170 lines
}
```

### 5. New Keyboard Controls (~20 lines)

Add to WM_KEYDOWN handler:
```c
else if (wParam == 'O') {
    ssaoEnabled = !ssaoEnabled;
    printf("SSAO: %s\n", ssaoEnabled ? "ON" : "OFF");
}
else if (wParam == 'K') {
    ssaoRadius += 0.05f;
    printf("SSAO radius: %.2f\n", ssaoRadius);
}
else if (wParam == 'L') {
    ssaoRadius -= 0.05f;
    if (ssaoRadius < 0.1f) ssaoRadius = 0.1f;
    printf("SSAO radius: %.2f\n", ssaoRadius);
}
```

### 6. Cleanup (~30 lines)

Add to cleanup section:
```c
// Delete G-buffer
glDeleteFramebuffers(1, &gBuffer);
glDeleteTextures(1, &gPosition);
glDeleteTextures(1, &gNormal);
glDeleteTextures(1, &gAlbedoSpec);
glDeleteRenderbuffers(1, &gDepth);

// Delete SSAO buffers
glDeleteFramebuffers(1, &ssaoFBO);
glDeleteFramebuffers(1, &ssaoBlurFBO);
glDeleteTextures(1, &ssaoColorBuffer);
glDeleteTextures(1, &ssaoColorBufferBlur);
glDeleteTextures(1, &noiseTexture);

// Delete SSAO shaders
glDeleteProgram(geometryShader);
glDeleteProgram(ssaoShader);
glDeleteProgram(ssaoBlurShader);
glDeleteProgram(lightingShader);
```

## Build Script Changes

No changes needed to `build_engine_mvp.bat` - all SSAO code is in demo file.

## Performance Impact

**Before SSAO:**
- Frame time: ~15ms @ 1920x1080
- FPS: ~66

**After SSAO (estimated):**
- Geometry pass: +1ms
- SSAO pass: +2ms
- Blur pass: +0.5ms
- Lighting pass: +1ms
- **Total: ~20ms**
- **Target FPS: ~50** (still very smooth)

Can optimize if needed by:
- Reducing SSAO samples (64 → 32)
- Reducing kernel size
- Half-resolution SSAO

## Alternative: Simpler Approach

If full SSAO is too complex for now, we can implement a **simplified screen-space AO**:

### "Lite SSAO" (~300 lines instead of 1100)
- Skip G-buffer, use depth buffer only
- Simpler sampling pattern (16 samples instead of 64)
- Single-pass AO calculation
- No blur pass (accept some noise)
- Apply as darkening factor in existing PBR shader

**Pros:**
- Much faster to implement (30 min vs 2 hours)
- Still looks significantly better than no AO
- ~70% of the visual quality for 25% of the code

**Cons:**
- More noise
- Less accurate
- Not as "raytracing-like"

## Recommendation

Given the scope, I suggest:

**Option A: Full SSAO Implementation** ✅
- Proper deferred rendering
- Best visual quality
- ~2 hours to implement and test
- This is the "right" way

**Option B: Lite SSAO**
- Quick win, still impressive
- ~30 minutes to implement
- Can upgrade to full SSAO later

**Option C: Other enhancements first**
- Better shadow mapping (PCF improvements)
- Environment maps for reflections
- Then come back to SSAO

**What would you like me to proceed with?**
