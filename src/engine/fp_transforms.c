#include "fp_graphics_engine.h"
#include "fp_core.h"
#include <math.h>

/**
 * Implements matrix multiplication by composing the existing, hand-optimized
 * fp_mat4_mul function from the core assembly library. This adheres to the
 * philosophy of "Composition over Re-implementation". The fp_mat4_mul is the
 * correct primitive for this operation.
 */
void fp_mat4_mul_pure(Mat4* out, const Mat4* a, const Mat4* b) {
    // Delegate to the highly optimized assembly primitive.
    fp_mat4_mul(out, a, b);
}

void fp_transform_to_matrix(Mat4* out, const Transform* t) {
    Mat4 trans_mat, rot_mat, scale_mat;
    Mat4 temp_mat;

    // 1. Create individual transformation matrices
    fp_mat4_translation(&trans_mat, t->position.x, t->position.y, t->position.z);
    fp_mat4_rotation_euler(&rot_mat, t->euler_rotation.x, t->euler_rotation.y, t->euler_rotation.z);
    fp_mat4_scale(&scale_mat, t->scale.x, t->scale.y, t->scale.z);

    // 2. Compose them in T * R * S order
    // temp = Rotate * Scale
    fp_mat4_mul_pure(&temp_mat, &rot_mat, &scale_mat);
    // out = Translate * temp
    fp_mat4_mul_pure(out, &trans_mat, &temp_mat);
}

void fp_view_matrix(Mat4* out, const Camera* cam) {
    // For simplicity, we'll have the camera always look at the world origin.
    // The 'up' vector is (0, 1, 0).
    fp_mat4_lookat(out,
                   cam->transform.position.x, cam->transform.position.y, cam->transform.position.z,
                   0.0f, 0.0f, 0.0f,
                   0.0f, 1.0f, 0.0f);
}

void fp_projection_matrix(Mat4* out, const CameraProjection* proj) {
    fp_mat4_perspective(out, proj->fov_radians, proj->aspect_ratio, proj->near_plane, proj->far_plane);
}

void fp_get_mvp_matrix(Mat4* out_mvp, const Mat4* model, const Mat4* view, const Mat4* proj) {
    Mat4 view_model;
    // temp = View * Model
    fp_mat4_mul_pure(&view_model, view, model);
    // out = Projection * temp
    fp_mat4_mul_pure(out_mvp, proj, &view_model);
}
