# Engine MVP Demo - ECS Architecture Showcase

## Overview

`demo_engine_mvp.c` is a modern 3D rendering demo showcasing the new Entity Component System (ECS) architecture integrated with OpenGL 3.3.

## Features

### Architecture
- **Entity Component System (ECS)**: Data-oriented design with sparse sets for O(1) component operations
- **Modern OpenGL 3.3**: Programmable pipeline with vertex/fragment shaders
- **Component-Based Design**: Modular entities with Transform, Mesh, Material, Light, and Camera components

### Rendering
- **500 Cube Entities**: Each with unique PBR materials arranged in a grid
- **PBR-like Shading**: Physically-based rendering with metallic/roughness workflow
- **Dual Directional Lights**: Main light (warm) and fill light (cool blue)
- **Procedural Animation**: Cubes rotate using Perlin noise for organic movement
- **Camera System**: First-person camera with WASD movement controls

### Performance
- **Target**: 60 FPS @ 1920x1080 resolution
- **Optimizations**:
  - Shared mesh data (single cube mesh for all entities)
  - Cache-friendly component storage
  - Efficient sparse set lookups

## Building

### Prerequisites
- GCC (MinGW64 on Windows)
- OpenGL 3.3+ compatible graphics card
- Windows 10/11

### Compile

#### Option 1: Using the batch script
```cmd
build_engine_mvp.bat
```

#### Option 2: Manual compilation
```cmd
gcc demo_engine_mvp.c src\engine\ecs.c src\engine\gl_extensions.c ^
    -o demo_engine_mvp.exe ^
    -I.\include ^
    -lopengl32 -lgdi32 -lm -lglu32
```

#### Option 3: Using Make
```bash
make engine_demo
```

## Running

Simply execute the compiled binary:
```cmd
demo_engine_mvp.exe
```

### Controls
- **W/A/S/D**: Camera movement (forward/left/backward/right)
- **ESC**: Exit application

## Architecture Details

### ECS Components Used

1. **TransformComponent**
   - Position, rotation (quaternion), scale
   - Cached local and world matrices
   - Dirty flag for optimized updates

2. **MeshComponent**
   - VAO, VBO, EBO references
   - Vertex/index counts
   - Shared across multiple entities

3. **MaterialComponent**
   - Albedo color (base color)
   - Metallic property (0 = dielectric, 1 = metal)
   - Roughness property (0 = smooth, 1 = rough)
   - Ambient occlusion
   - Texture map support (future)

4. **LightComponent**
   - Type (directional/point/spot)
   - Color and intensity
   - Shadow casting enabled/disabled
   - Shadow map framebuffer (future)

5. **CameraComponent**
   - Perspective/orthographic projection
   - FOV, aspect ratio, near/far planes
   - View and projection matrices

### Scene Structure

```
World
├── 500 Cube Entities
│   ├── Transform (grid positions, animated rotations)
│   ├── Mesh (shared cube geometry)
│   └── Material (varying metallic/roughness values)
├── 2 Light Entities
│   ├── Light 1: Main directional (top-right, warm)
│   └── Light 2: Fill directional (bottom-left, cool blue)
└── 1 Camera Entity
    ├── Transform (position controlled by player)
    └── Camera (perspective projection, view matrix)
```

### Rendering Pipeline

1. **Update Phase**
   - Handle input (WASD movement)
   - Update camera transform and view matrix
   - Animate cube rotations with Perlin noise
   - Update dirty transform matrices

2. **Render Phase**
   - Clear framebuffer
   - Bind PBR shader
   - Set camera uniforms (view, projection, position)
   - Set light uniforms (2 directional lights)
   - For each entity with Mesh + Material + Transform:
     - Update model matrix if dirty
     - Set material uniforms (albedo, metallic, roughness)
     - Draw mesh geometry
   - Swap buffers

3. **Performance Tracking**
   - FPS counter (updates every second)
   - Frame time measurement
   - Entity count display

## Code Organization

```
demo_engine_mvp.c
├── Perlin Noise Implementation (for animation)
├── Shader Creation Utilities
├── PBR Vertex/Fragment Shaders (embedded GLSL)
├── Mesh Creation (cube geometry with normals)
├── ECS Scene Setup (entities, components)
├── Camera Update System
├── Animation System (Perlin-based rotation)
├── Rendering System (OpenGL draw calls)
├── OpenGL Initialization
├── Window Procedure (Win32)
└── Main Entry Point (WinMain)
```

## Technical Highlights

### Memory Efficiency
- **Sparse Sets**: O(1) component add/remove/get operations
- **Packed Arrays**: Cache-friendly iteration over active components
- **Shared Meshes**: Single cube mesh reused by all 500 entities

### Rendering Optimizations
- **Minimal State Changes**: Shader bound once per frame
- **Batch Uniforms**: Camera and lights set once
- **Indexed Drawing**: 36 indices instead of 72 vertices per cube

### Animation Quality
- **Perlin Noise**: Smooth, organic rotation patterns
- **Per-Entity Noise**: Each cube has unique animation based on position
- **Quaternion Rotations**: Smooth interpolation without gimbal lock

## Future Enhancements

### Short-term
- Shadow mapping implementation
- SSAO (Screen-Space Ambient Occlusion)
- Post-processing (FXAA, bloom, tone mapping)

### Medium-term
- Texture loading and mapping
- Normal mapping for surface detail
- Point and spot light support
- Multiple camera support

### Long-term
- Deferred rendering for many lights
- Frustum culling for large scenes
- LOD (Level of Detail) system
- Physics integration

## Performance Expectations

### Target Hardware
- GPU: GTX 1060 / RX 580 or better
- CPU: Intel i5 / Ryzen 5 or better
- RAM: 4GB+

### Expected Performance
- **1920x1080**: 60+ FPS
- **2560x1440**: 45+ FPS
- **3840x2160**: 30+ FPS

### Bottlenecks
- **Memory Bandwidth**: 500 cubes = 18,000 triangles/frame
- **Fill Rate**: 1920x1080 = 2M pixels/frame
- **CPU Overhead**: Transform updates, matrix calculations

## Comparison to Legacy Demos

### vs. demo_opengl_production.c
- **+** ECS architecture (more maintainable)
- **+** Component-based entities (more flexible)
- **+** Cleaner separation of concerns
- **-** No MSAA yet (planned)
- **-** No environment mapping yet (planned)

### vs. demo_opengl_ultra.c
- **+** Modern architecture
- **+** Better code organization
- **+** More entities (500 vs 300)
- **-** No cubemaps yet
- **-** No advanced effects yet

## License

Part of the FP-ASM library project. See main CLAUDE.md for details.

## Authors

- ECS Architecture: Modern engine refactoring
- PBR Shaders: Simplified metallic/roughness workflow
- Animation System: Perlin noise integration
