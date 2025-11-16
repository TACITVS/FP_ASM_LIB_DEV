#include "application.h"
#include "fp_math.h"
#include <math.h>

CameraControls update_camera_controls(CameraControls c, const UserInput* i) {
    if (i->left_mouse_down) {
        c.yaw_rad -= i->dx * 0.01f;
        c.pitch_rad -= i->dy * 0.01f;
    }
    if (i->right_mouse_down) {
        Vec3f forward = { sinf(c.yaw_rad) * cosf(c.pitch_rad), sinf(c.pitch_rad), cosf(c.yaw_rad) * cosf(c.pitch_rad) };
        Vec3f right = vec3f_normalize(vec3f_cross(forward, (Vec3f){0, 1, 0}));
        Vec3f up = vec3f_normalize(vec3f_cross(right, forward));
        float pan_speed = 0.005f * c.distance;
        c.target = vec3f_add(c.target, vec3f_scale(right, (float)-i->dx * pan_speed));
        c.target = vec3f_add(c.target, vec3f_scale(up, (float)i->dy * pan_speed));
    }
    if (c.pitch_rad > M_PI/2.0f-0.1f) c.pitch_rad = M_PI/2.0f-0.1f;
    if (c.pitch_rad < -M_PI/2.0f+0.1f) c.pitch_rad = -M_PI/2.0f+0.1f;
    if (i->scroll_delta != 0) { c.distance -= i->scroll_delta * 0.05f; }
    if (c.distance < 2.0f) c.distance = 2.0f;
    if (c.distance > 50.0f) c.distance = 50.0f;
    return c;
}

AppState update_app_state(AppState s, const UserInput* i) {
    s.camera_controls = update_camera_controls(s.camera_controls, i);
    s.camera.transform.position = (Vec3f){
        s.camera_controls.target.x + s.camera_controls.distance * sinf(s.camera_controls.yaw_rad) * cosf(s.camera_controls.pitch_rad),
        s.camera_controls.target.y + s.camera_controls.distance * sinf(s.camera_controls.pitch_rad),
        s.camera_controls.target.z + s.camera_controls.distance * cosf(s.camera_controls.yaw_rad) * cosf(s.camera_controls.pitch_rad)
    };
    // Animate the cubes
    s.objects[0].transform.euler_rotation.y += 0.005f;
    s.objects[0].transform.euler_rotation.x += 0.002f;
    s.objects[1].transform.euler_rotation.y -= 0.003f;
    s.objects[2].transform.euler_rotation.x += 0.004f;
    s.objects[2].transform.euler_rotation.z -= 0.006f;
    return s;
}

AppState init_app_state() {
    AppState s = {
        .camera_controls = {10.0f, 0.5f, -0.3f, {0,0,0}},
        .camera = {.projection = {M_PI/2.5f, (float)WINDOW_WIDTH/WINDOW_HEIGHT, 0.1f, 100.0f}},
        .object_count = 4,
        .light = { .position = {-10.0f, 20.0f, -10.0f}, .target = {0,0,0}, .color = {1,1,1} },
        .cube_mesh = fp_mesh_create_cube(),
        .plane_mesh = fp_mesh_create_plane(20.0f)
    };
    s.objects[0] = (SceneObject){ .transform = {{0,0,0},{1,1,1},{0,0,0}}, .mesh = &s.cube_mesh, .material_color = {1.0f, 1.0f, 1.0f} };
    s.objects[1] = (SceneObject){ .transform = {{-3,0,2},{0.7,0.7,0.7},{0,0,0}}, .mesh = &s.cube_mesh, .material_color = {0.8f, 0.2f, 0.2f} };
    s.objects[2] = (SceneObject){ .transform = {{3,0,-2},{0.8,0.8,0.8},{0,0,0}}, .mesh = &s.cube_mesh, .material_color = {0.2f, 0.8f, 0.2f} };
    s.objects[3] = (SceneObject){ .transform = {{0,-1.5f,0},{1,1,1},{0,0,0}}, .mesh = &s.plane_mesh, .material_color = {0.5f, 0.5f, 0.5f} };
    return s;
}

int main(int argc, char** argv) {
    return platform_run();
}