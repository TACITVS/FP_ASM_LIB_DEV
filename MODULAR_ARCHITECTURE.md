# Modular Graphics Engine Architecture

## Core Principle: Independent Feature Toggles

Every visual enhancement must be **independently toggleable** without breaking other features. Users can scale from bare-minimum (60+ FPS on low-end) to raytracing-quality (30+ FPS on high-end).

## Quality Settings Structure

```c
typedef struct {
    // Lighting & Shading
    BOOL enable_pbr;              // PBR vs simple Blinn-Phong
    BOOL enable_ssao;             // Ambient occlusion
    BOOL enable_shadows;          // Shadow mapping
    BOOL enable_soft_shadows;     // PCF filtering (requires enable_shadows)

    // Anti-Aliasing
    BOOL enable_msaa;             // Hardware MSAA (8x)
    BOOL enable_fxaa;             // Post-process FXAA

    // Post-Processing
    BOOL enable_bloom;            // HDR bloom
    BOOL enable_tone_mapping;     // HDR to LDR conversion
    BOOL enable_gamma_correction; // sRGB conversion

    // Advanced (Future)
    BOOL enable_ssr;              // Screen-space reflections
    BOOL enable_gi;               // Global illumination
    BOOL enable_dof;              // Depth of field

    // Performance Settings
    int ssao_samples;             // 8, 16, 32, 64
    int shadow_resolution;        // 512, 1024, 2048, 4096
    int msaa_samples;             // 0, 2, 4, 8
} RenderQualitySettings;
```

## Rendering Pipeline (Modular)

```
BASE PASS (Always runs):
  - Clear screen
  - For each mesh:
    - If enable_pbr:
        Use PBR shader with Cook-Torrance
      Else:
        Use simple Blinn-Phong shader
    - Draw mesh

SHADOW PASS (if enable_shadows):
  - Render depth from light POV
  - If enable_soft_shadows:
      Apply PCF filtering

SSAO PASS (if enable_ssao):
  - Calculate ambient occlusion
  - Samples: ssao_samples (8-64)

POST-PROCESS (if any enabled):
  - If enable_bloom:
      Extract bright regions → blur → combine
  - If enable_fxaa:
      Apply edge detection AA
  - If enable_tone_mapping:
      Apply Reinhard/ACES operator
  - If enable_gamma_correction:
      Convert linear → sRGB

PRESENT
```

## Preset Quality Levels

```c
// Low-End (60+ FPS)
RenderQualitySettings PRESET_LOW = {
    .enable_pbr = FALSE,           // Simple lighting
    .enable_ssao = FALSE,
    .enable_shadows = FALSE,
    .enable_msaa = FALSE,
    .enable_tone_mapping = TRUE,   // Minimal overhead
    .enable_gamma_correction = TRUE,
    .ssao_samples = 0,
    .shadow_resolution = 0,
    .msaa_samples = 0
};

// Medium (45-60 FPS)
RenderQualitySettings PRESET_MEDIUM = {
    .enable_pbr = TRUE,
    .enable_ssao = TRUE,
    .enable_shadows = TRUE,
    .enable_soft_shadows = FALSE,  // Simple shadows
    .enable_msaa = FALSE,
    .enable_fxaa = TRUE,           // Cheaper AA
    .enable_tone_mapping = TRUE,
    .enable_gamma_correction = TRUE,
    .ssao_samples = 16,            // Lower sample count
    .shadow_resolution = 1024,
    .msaa_samples = 0
};

// High (30-45 FPS)
RenderQualitySettings PRESET_HIGH = {
    .enable_pbr = TRUE,
    .enable_ssao = TRUE,
    .enable_shadows = TRUE,
    .enable_soft_shadows = TRUE,
    .enable_msaa = TRUE,
    .enable_bloom = TRUE,
    .enable_tone_mapping = TRUE,
    .enable_gamma_correction = TRUE,
    .ssao_samples = 32,
    .shadow_resolution = 2048,
    .msaa_samples = 4
};

// Ultra (Raytracing-like, 30 FPS)
RenderQualitySettings PRESET_ULTRA = {
    .enable_pbr = TRUE,
    .enable_ssao = TRUE,
    .enable_shadows = TRUE,
    .enable_soft_shadows = TRUE,
    .enable_msaa = TRUE,
    .enable_fxaa = TRUE,           // Both AA methods
    .enable_bloom = TRUE,
    .enable_tone_mapping = TRUE,
    .enable_gamma_correction = TRUE,
    .ssao_samples = 64,
    .shadow_resolution = 4096,
    .msaa_samples = 8
};
```

## Keyboard Controls (Toggle Individual Features)

```
QUALITY PRESETS:
  F1 - Low preset
  F2 - Medium preset
  F3 - High preset
  F4 - Ultra preset

INDIVIDUAL TOGGLES:
  O - Toggle SSAO
  P - Toggle PBR (vs simple lighting)
  H - Toggle shadows
  B - Toggle bloom
  M - Toggle MSAA
  G - Toggle gamma correction

  CTRL+W - Toggle wireframe (existing)
  SPACE  - Pause animation (existing)
```

## Functional Programming Integration

Where possible, use FP principles:

```c
// Pure function for SSAO kernel generation
Vec3* fp_generate_ssao_kernel(int sample_count) {
    // No side effects
    // Deterministic output
    Vec3* kernel = malloc(sample_count * sizeof(Vec3));
    // ... generate samples ...
    return kernel;
}

// Pure function for noise generation
float* fp_generate_noise_pattern(int width, int height) {
    // No side effects
    float* noise = malloc(width * height * 3 * sizeof(float));
    // ... generate noise ...
    return noise;
}

// Functional approach to settings
RenderQualitySettings fp_clamp_settings(RenderQualitySettings input) {
    // Ensure dependencies are met
    if (!input.enable_shadows) {
        input.enable_soft_shadows = FALSE;
    }
    if (input.ssao_samples < 8) {
        input.enable_ssao = FALSE;
    }
    return input; // Immutable transformation
}
```

## Implementation Strategy for Lite SSAO

Since we're doing Lite SSAO first, here's how it fits into the modular system:

### Phase 1: Add Quality Settings Structure (Now)
- Define `RenderQualitySettings` struct
- Add global `quality_settings` variable
- Add preset definitions
- Add keyboard controls for toggles

### Phase 2: Implement Lite SSAO (Next)
- Add SSAO as **optional post-process pass**
- Only runs if `quality_settings.enable_ssao == TRUE`
- Uses `quality_settings.ssao_samples` for sample count
- Falls back gracefully if disabled

### Phase 3: Future Enhancements
- Each new feature checks its flag before rendering
- No feature depends on another (except explicit dependencies)
- Settings can be saved/loaded from file

## Code Organization

```
demo_engine_mvp.c structure:

1. [Structures & Globals]
   - RenderQualitySettings definition
   - Global settings variable
   - Feature-specific state (FBOs, shaders, etc.)

2. [Initialization]
   - setup_quality_system()
   - setup_ssao() // Only if enabled
   - setup_shadows() // Only if enabled
   - etc.

3. [Render Loop]
   - render_base_geometry()
   - if (enable_ssao) render_ssao_pass()
   - if (enable_shadows) render_shadow_pass()
   - if (enable_bloom) render_bloom_pass()
   - render_final_composite()

4. [Controls]
   - handle_quality_toggles()
   - handle_preset_selection()

5. [Cleanup]
   - Each feature cleans up its own resources
```

## Performance Budget

| Quality | Target FPS | Budget/Frame |
|---------|------------|--------------|
| Low     | 60+        | <16.67ms     |
| Medium  | 45-60      | 16-22ms      |
| High    | 30-45      | 22-33ms      |
| Ultra   | 30+        | 33ms         |

## Summary

This architecture ensures:
1. ✅ **Modularity** - Each feature is independent
2. ✅ **Scalability** - From bare mesh to raytracing
3. ✅ **FP-first** - Use pure functions where applicable
4. ✅ **User control** - Toggle everything via keyboard
5. ✅ **Professional** - Like AAA game quality settings
6. ✅ **Maintainable** - Easy to add new features

**Ready to implement Lite SSAO as the first modular feature?**
