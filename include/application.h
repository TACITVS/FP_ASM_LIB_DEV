#ifndef APPLICATION_H
#define APPLICATION_H

#include "fp_graphics_engine.h"
#include "fp_mesh_generation.h"
#include "platform.h"

typedef struct {
    Transform transform;
    FP_MeshData* mesh;
    Vec3f material_color;
} SceneObject;

typedef struct {
    float distance;
    float yaw_rad;
    float pitch_rad;
    Vec3f target;
} CameraControls;

typedef struct {
    Vec3f position;
    Vec3f target;
    Vec3f color;
} Light;

typedef struct AppState {
    Camera camera;
    CameraControls camera_controls;
    SceneObject objects[4];
    int object_count;
    Light light;
    FP_MeshData cube_mesh;
    FP_MeshData plane_mesh;
} AppState;

// Functions implemented by the application layer (demo_fp_opengl.c)
AppState init_app_state();
AppState update_app_state(AppState s, const UserInput* i);

#endif // APPLICATION_H
