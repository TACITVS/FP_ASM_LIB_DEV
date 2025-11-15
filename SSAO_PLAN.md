# SSAO Implementation Plan

## Overview
Adding Screen-Space Ambient Occlusion to the FP-ASM Engine MVP to achieve raytracing-like visual quality.

## Architecture Changes

### Current Pipeline (Forward Rendering):
```
For each object:
  - Bind PBR shader
  - Set uniforms
  - Draw with lighting
↓
Post-process (tone mapping)
↓
Display
```

### New Pipeline (Deferred + SSAO):
```
GEOMETRY PASS:
  For each object:
    - Render to G-buffer (position, normal, albedo, depth)

SSAO PASS:
  - Sample G-buffer positions/normals
  - For each pixel:
    - Generate hemisphere samples around surface
    - Test occlusion by comparing sample depths
    - Accumulate occlusion factor
  - Output: Noisy AO texture

BLUR PASS:
  - Bilateral blur on SSAO texture
  - Preserve edges using normal/depth info
  - Output: Smooth AO texture

LIGHTING PASS:
  - Read G-buffer + AO texture
  - Apply PBR lighting
  - Multiply by AO factor
  - Output: HDR color

POST-PROCESS:
  - Tone mapping
  - Gamma correction
↓
Display
```

## Required Components

### 1. G-Buffer (4 textures)
- **gPosition** (RGB32F): World-space position
- **gNormal** (RGB16F): View-space normal
- **gAlbedo** (RGBA8): Albedo + roughness
- **gDepth** (DEPTH24_STENCIL8): Depth buffer

### 2. SSAO Framebuffer
- **ssaoColor** (R8): Occlusion factor [0,1]

### 3. SSAO Blur Framebuffer
- **ssaoBlur** (R8): Smoothed occlusion

### 4. HDR Framebuffer
- **hdrColor** (RGBA16F): HDR lit scene

### 5. SSAO Kernel
- 64 random samples in hemisphere
- Distributed using lerp to concentrate near origin
- Formula: `lerp(0.1, 1.0, scale^2)` where scale = i/64

### 6. Noise Texture
- 4x4 random rotation vectors (tangent space)
- Tiled across screen to rotate SSAO kernel

## Shader Code

### Geometry Pass Vertex Shader
```glsl
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;
out vec3 Normal;

void main() {
    vec4 viewPos = view * model * vec4(aPos, 1.0);
    FragPos = viewPos.xyz;
    Normal = mat3(transpose(inverse(view * model))) * aNormal;
    gl_Position = projection * viewPos;
}
```

### Geometry Pass Fragment Shader
```glsl
#version 330 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 albedo;
uniform float metallic;
uniform float roughness;

void main() {
    gPosition = FragPos;
    gNormal = normalize(Normal);
    gAlbedoSpec.rgb = albedo;
    gAlbedoSpec.a = roughness;
}
```

### SSAO Shader
```glsl
#version 330 core
out float FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D texNoise;
uniform vec3 samples[64];
uniform mat4 projection;

// Parameters
const int kernelSize = 64;
const float radius = 0.5;
const float bias = 0.025;

// Tile noise texture
const vec2 noiseScale = vec2(1920.0/4.0, 1080.0/4.0);

void main() {
    vec3 fragPos = texture(gPosition, TexCoords).xyz;
    vec3 normal = normalize(texture(gNormal, TexCoords).rgb);
    vec3 randomVec = normalize(texture(texNoise, TexCoords * noiseScale).xyz);

    // Create TBN matrix
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    // Sample hemisphere
    float occlusion = 0.0;
    for(int i = 0; i < kernelSize; ++i) {
        // Get sample position
        vec3 samplePos = TBN * samples[i];
        samplePos = fragPos + samplePos * radius;

        // Project to screen space
        vec4 offset = vec4(samplePos, 1.0);
        offset = projection * offset;
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;

        // Get sample depth
        float sampleDepth = texture(gPosition, offset.xy).z;

        // Range check & accumulate
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
    }

    occlusion = 1.0 - (occlusion / kernelSize);
    FragColor = occlusion;
}
```

### Blur Shader (Bilateral)
```glsl
#version 330 core
out float FragColor;

in vec2 TexCoords;

uniform sampler2D ssaoInput;

void main() {
    vec2 texelSize = 1.0 / vec2(textureSize(ssaoInput, 0));
    float result = 0.0;

    // Simple box blur
    for (int x = -2; x < 2; ++x) {
        for (int y = -2; y < 2; ++y) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(ssaoInput, TexCoords + offset).r;
        }
    }

    FragColor = result / (4.0 * 4.0);
}
```

### Lighting Pass Shader
```glsl
#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform sampler2D ssaoBlur;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

void main() {
    vec3 FragPos = texture(gPosition, TexCoords).xyz;
    vec3 Normal = texture(gNormal, TexCoords).rgb;
    vec3 Albedo = texture(gAlbedoSpec, TexCoords).rgb;
    float Roughness = texture(gAlbedoSpec, TexCoords).a;
    float AO = texture(ssaoBlur, TexCoords).r;

    // ... PBR lighting calculation ...

    // Apply AO
    vec3 ambient = vec3(0.03) * Albedo * AO;
    vec3 color = ambient + lighting;

    FragColor = vec4(color, 1.0);
}
```

## Performance Considerations

- **G-Buffer size**: 1920x1080 * (3+3+4+4) = ~31MB VRAM
- **SSAO pass**: ~1-2ms at 1080p with 64 samples
- **Blur pass**: ~0.5ms
- **Total overhead**: ~2-3ms per frame

Target: Still maintain 60 FPS (16.67ms budget)

## Toggle Controls

Add keyboard shortcut to compare:
- **Key 'O'**: Toggle SSAO on/off (to compare visual quality)
- Console output: "SSAO: ON" / "SSAO: OFF"

## Implementation Steps

1. ✅ Create plan document
2. ⏳ Add G-buffer framebuffers and textures
3. ⏳ Generate SSAO kernel and noise texture
4. ⏳ Create geometry pass shader
5. ⏳ Create SSAO shader
6. ⏳ Create blur shader
7. ⏳ Create lighting pass shader
8. ⏳ Update render loop with new pipeline
9. ⏳ Add SSAO toggle control
10. ⏳ Test and optimize

## Expected Visual Result

Before SSAO:
- Flat ambient lighting
- No contact shadows
- Less depth perception

After SSAO:
- Dark areas in crevices and corners
- Contact shadows where objects meet
- Significantly improved depth perception
- More realistic, "grounded" look
- Closer to raytracing quality
