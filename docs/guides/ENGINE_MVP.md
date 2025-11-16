# FP-ASM Game Engine MVP Architecture

## Goal
Create a production-quality 3D game engine with raytracing-like visuals at 60 FPS using modern OpenGL 3.3+ and ECS architecture.

---

## MVP Feature Set

### Core Systems (✅ COMPLETE)
1. **ECS (Entity Component System)**
   - Sparse set architecture for O(1) operations
   - Components: Transform, Mesh, Material, Light, Camera
   - Supports 10,000+ entities
   - Cache-friendly iteration

### Graphics Systems (🚧 IN PROGRESS)
2. **Modern OpenGL Renderer**
   - OpenGL 3.3+ with GLSL shaders
   - VBO/VAO/EBO for efficient mesh rendering
   - Framebuffer objects for post-processing

3. **PBR (Physically-Based Rendering)**
   - Metallic/roughness workflow
   - Fresnel reflections (Schlick's approximation)
   - Energy-conserving BRDF
   - Image-Based Lighting (IBL) - basic

4. **Shadow Mapping with PCF**
   - Percentage Closer Filtering for soft shadows
   - 2048x2048 shadow maps per light
   - Configurable shadow bias
   - Multiple light shadow support

5. **Post-Processing**
   - FXAA (Fast Approximate Anti-Aliasing)
   - Tone mapping (Reinhard/ACES)
   - Gamma correction
   - Optional: Bloom (additive glow)

---

## Rendering Pipeline

```
Frame Start
    ↓
[Shadow Pass]
    - For each light that casts shadows:
        - Bind shadow framebuffer
        - Render scene from light's POV (depth only)
        - Store depth in shadow map texture
    ↓
[Geometry Pass]
    - Bind main framebuffer (HDR)
    - Clear color + depth
    - For each renderable entity:
        - Get Transform + Mesh + Material components
        - Set PBR uniforms (albedo, metallic, roughness, ao)
        - Set transform matrices (model, view, projection)
        - Bind shadow maps
        - Draw mesh with PBR shader
    ↓
[Post-Processing Pass]
    - Bind post-process framebuffer
    - Render fullscreen quad with screen texture
    - Apply FXAA (edge detection + blur)
    - Apply tone mapping (HDR → LDR)
    - Apply gamma correction (linear → sRGB)
    ↓
[Present to Screen]
    - Blit final framebuffer to default framebuffer
    ↓
Frame End
```

---

## PBR Shader Details

### Vertex Shader
- Input: Position, Normal, UV, Tangent
- Output: World position, View-space position, Normal, UV
- Transforms: Model → World → View → Clip space

### Fragment Shader (Simplified Cook-Torrance BRDF)

```glsl
// Material properties
vec3 albedo;
float metallic;
float roughness;
float ao;

// Lighting calculation for each light
for (int i = 0; i < lightCount; i++) {
    // Light direction and attenuation
    vec3 L = normalize(lightPosition[i] - worldPos);
    float distance = length(lightPosition[i] - worldPos);
    float attenuation = 1.0 / (distance * distance);

    // View and half vectors
    vec3 V = normalize(viewPos - worldPos);
    vec3 H = normalize(V + L);

    // Fresnel (Schlick approximation)
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = F0 + (1.0 - F0) * pow(1.0 - max(dot(H, V), 0.0), 5.0);

    // Normal Distribution Function (GGX/Trowbridge-Reitz)
    float NDF = DistributionGGX(N, H, roughness);

    // Geometry Function (Smith's Schlick-GGX)
    float G = GeometrySmith(N, V, L, roughness);

    // Cook-Torrance BRDF
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    // Energy conservation
    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);

    // Lambertian diffuse
    vec3 diffuse = kD * albedo / PI;

    // Shadow factor (PCF)
    float shadow = CalculateShadowPCF(lightSpacePos[i], shadowMap[i]);

    // Combine
    float NdotL = max(dot(N, L), 0.0);
    Lo += (diffuse + specular) * lightColor[i] * NdotL * attenuation * shadow;
}

// Ambient (simple approximation, can upgrade to IBL)
vec3 ambient = vec3(0.03) * albedo * ao;

// Final color
vec3 color = ambient + Lo;
```

---

## Shadow Mapping with PCF

### Shadow Map Generation (Shadow Pass)
```glsl
// Vertex shader
gl_Position = lightSpaceMatrix * model * vec4(position, 1.0);

// Fragment shader
// (depth written automatically to depth buffer)
```

### Shadow Sampling (Main Pass)
```glsl
float CalculateShadowPCF(vec4 fragPosLightSpace, sampler2D shadowMap) {
    // Perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;  // Transform to [0,1]

    // Get current depth
    float currentDepth = projCoords.z;

    // Bias to prevent shadow acne
    float bias = 0.005;

    // PCF (Percentage Closer Filtering) - 3x3 kernel
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;  // Average

    // Outside shadow frustum = no shadow
    if(projCoords.z > 1.0) shadow = 0.0;

    return shadow;
}
```

---

## FXAA (Fast Approximate Anti-Aliasing)

FXAA detects edges and applies directional blur to smooth them out.

```glsl
// Simplified FXAA (single-pass)
vec3 FXAA(sampler2D tex, vec2 uv, vec2 texelSize) {
    // Sample center and neighbors
    vec3 rgbNW = texture(tex, uv + vec2(-1.0, -1.0) * texelSize).rgb;
    vec3 rgbNE = texture(tex, uv + vec2( 1.0, -1.0) * texelSize).rgb;
    vec3 rgbSW = texture(tex, uv + vec2(-1.0,  1.0) * texelSize).rgb;
    vec3 rgbSE = texture(tex, uv + vec2( 1.0,  1.0) * texelSize).rgb;
    vec3 rgbM  = texture(tex, uv).rgb;

    // Luma (perceived brightness)
    float lumaNW = dot(rgbNW, vec3(0.299, 0.587, 0.114));
    float lumaNE = dot(rgbNE, vec3(0.299, 0.587, 0.114));
    float lumaSW = dot(rgbSW, vec3(0.299, 0.587, 0.114));
    float lumaSE = dot(rgbSE, vec3(0.299, 0.587, 0.114));
    float lumaM  = dot(rgbM,  vec3(0.299, 0.587, 0.114));

    // Edge detection
    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
    float lumaRange = lumaMax - lumaMin;

    // Skip if edge too weak
    if(lumaRange < max(0.0625, lumaMax * 0.125)) {
        return rgbM;
    }

    // Directional blur based on edge direction
    // (full FXAA implementation)
    // ...

    return blendedColor;
}
```

---

## Performance Targets

### Target Hardware
- GPU: Any with OpenGL 3.3+ support (2010+)
- CPU: Multi-core recommended for ECS systems

### Performance Goals
- **60 FPS minimum** at 1920x1080
- **1000+ rendered entities** with frustum culling
- **<16.67ms frame time**:
  - Shadow pass: ~3ms (2 lights)
  - Geometry pass: ~8ms (1000 entities)
  - Post-processing: ~2ms
  - Buffer: ~3.67ms

### Optimizations
1. **Frustum culling** - Don't render off-screen objects
2. **Static batching** - Combine meshes with same material
3. **GPU instancing** - Render duplicates in single draw call (future)
4. **LOD system** - Lower detail for distant objects (future)

---

## File Structure

```
fp_asm_lib_dev/
├── include/
│   ├── ecs.h                      # ✅ ECS core API
│   ├── renderer_modern.h          # 🚧 Modern renderer API
│   └── gl_extensions.h            # 📋 OpenGL extension loading
├── src/
│   └── engine/
│       ├── ecs.c                  # ✅ ECS implementation
│       ├── renderer_modern.c      # 🚧 Renderer implementation
│       ├── shaders_embedded.c     # 📋 GLSL shaders (embedded)
│       └── gl_extensions.c        # 📋 wglGetProcAddress wrapper
├── demo_engine_mvp.c              # 📋 MVP demo application
└── build_engine_mvp.bat           # 📋 Build script
```

---

## Next Steps

1. ✅ Create ECS core (`ecs.h`, `ecs.c`)
2. 🚧 Create renderer header (`renderer_modern.h`)
3. 📋 Create OpenGL extension loader (`gl_extensions.h/c`)
4. 📋 Create embedded GLSL shaders (`shaders_embedded.c`)
5. 📋 Create renderer implementation (`renderer_modern.c`)
6. 📋 Create MVP demo (`demo_engine_mvp.c`)
7. 📋 Test and optimize for 60 FPS

---

## Post-MVP Enhancements

Once MVP is working at 60 FPS, add:
- **SSAO** (Screen-Space Ambient Occlusion)
- **Bloom** (HDR glow)
- **IBL** (Image-Based Lighting) with environment maps
- **SSR** (Screen-Space Reflections)
- **Particle systems**
- **Physics integration**
- **Audio system**
