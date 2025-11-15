/**
 * fp_matrix_ops.c - Comprehensive 3D Matrix Creation Operations
 *
 * Column-major layout (OpenGL compatible):
 * m[0..3]   = column 0 (x-axis for rotation matrices)
 * m[4..7]   = column 1 (y-axis for rotation matrices)
 * m[8..11]  = column 2 (z-axis for rotation matrices)
 * m[12..15] = column 3 (translation component)
 *
 * Coordinate system: Right-handed (OpenGL/Vulkan standard)
 */

#include "fp_core.h"
#include <math.h>

/* ========== BASIC TRANSFORMATIONS ========== */

void fp_mat4_translation(Mat4* out, float x, float y, float z) {
    out->m[0] = 1.0f;  out->m[4] = 0.0f;  out->m[8]  = 0.0f;  out->m[12] = x;
    out->m[1] = 0.0f;  out->m[5] = 1.0f;  out->m[9]  = 0.0f;  out->m[13] = y;
    out->m[2] = 0.0f;  out->m[6] = 0.0f;  out->m[10] = 1.0f;  out->m[14] = z;
    out->m[3] = 0.0f;  out->m[7] = 0.0f;  out->m[11] = 0.0f;  out->m[15] = 1.0f;
}

void fp_mat4_scale(Mat4* out, float sx, float sy, float sz) {
    out->m[0] = sx;    out->m[4] = 0.0f;  out->m[8]  = 0.0f;  out->m[12] = 0.0f;
    out->m[1] = 0.0f;  out->m[5] = sy;    out->m[9]  = 0.0f;  out->m[13] = 0.0f;
    out->m[2] = 0.0f;  out->m[6] = 0.0f;  out->m[10] = sz;    out->m[14] = 0.0f;
    out->m[3] = 0.0f;  out->m[7] = 0.0f;  out->m[11] = 0.0f;  out->m[15] = 1.0f;
}

void fp_mat4_scale_uniform(Mat4* out, float s) {
    fp_mat4_scale(out, s, s, s);
}

/* ========== ROTATIONS ========== */

void fp_mat4_rotation_x(Mat4* out, float angle_radians) {
    float c = cosf(angle_radians);
    float s = sinf(angle_radians);

    out->m[0] = 1.0f;  out->m[4] = 0.0f;  out->m[8]  = 0.0f;  out->m[12] = 0.0f;
    out->m[1] = 0.0f;  out->m[5] = c;     out->m[9]  = -s;    out->m[13] = 0.0f;
    out->m[2] = 0.0f;  out->m[6] = s;     out->m[10] = c;     out->m[14] = 0.0f;
    out->m[3] = 0.0f;  out->m[7] = 0.0f;  out->m[11] = 0.0f;  out->m[15] = 1.0f;
}

void fp_mat4_rotation_y(Mat4* out, float angle_radians) {
    float c = cosf(angle_radians);
    float s = sinf(angle_radians);

    out->m[0] = c;     out->m[4] = 0.0f;  out->m[8]  = s;     out->m[12] = 0.0f;
    out->m[1] = 0.0f;  out->m[5] = 1.0f;  out->m[9]  = 0.0f;  out->m[13] = 0.0f;
    out->m[2] = -s;    out->m[6] = 0.0f;  out->m[10] = c;     out->m[14] = 0.0f;
    out->m[3] = 0.0f;  out->m[7] = 0.0f;  out->m[11] = 0.0f;  out->m[15] = 1.0f;
}

void fp_mat4_rotation_z(Mat4* out, float angle_radians) {
    float c = cosf(angle_radians);
    float s = sinf(angle_radians);

    out->m[0] = c;     out->m[4] = -s;    out->m[8]  = 0.0f;  out->m[12] = 0.0f;
    out->m[1] = s;     out->m[5] = c;     out->m[9]  = 0.0f;  out->m[13] = 0.0f;
    out->m[2] = 0.0f;  out->m[6] = 0.0f;  out->m[10] = 1.0f;  out->m[14] = 0.0f;
    out->m[3] = 0.0f;  out->m[7] = 0.0f;  out->m[11] = 0.0f;  out->m[15] = 1.0f;
}

void fp_mat4_rotation_axis(Mat4* out, float x, float y, float z, float angle_radians) {
    // Normalize axis
    float len = sqrtf(x*x + y*y + z*z);
    if (len < 1e-8f) {
        fp_mat4_identity(out);
        return;
    }
    x /= len;
    y /= len;
    z /= len;

    float c = cosf(angle_radians);
    float s = sinf(angle_radians);
    float t = 1.0f - c;

    // Rodrigues' rotation formula
    out->m[0] = t*x*x + c;      out->m[4] = t*x*y - s*z;    out->m[8]  = t*x*z + s*y;    out->m[12] = 0.0f;
    out->m[1] = t*x*y + s*z;    out->m[5] = t*y*y + c;      out->m[9]  = t*y*z - s*x;    out->m[13] = 0.0f;
    out->m[2] = t*x*z - s*y;    out->m[6] = t*y*z + s*x;    out->m[10] = t*z*z + c;      out->m[14] = 0.0f;
    out->m[3] = 0.0f;           out->m[7] = 0.0f;           out->m[11] = 0.0f;           out->m[15] = 1.0f;
}

void fp_mat4_rotation_euler(Mat4* out, float pitch_x, float yaw_y, float roll_z) {
    Mat4 rx, ry, rz;
    fp_mat4_rotation_x(&rx, pitch_x);
    fp_mat4_rotation_y(&ry, yaw_y);
    fp_mat4_rotation_z(&rz, roll_z);

    Mat4 temp;
    fp_mat4_mul(&temp, &ry, &rx);    // temp = Ry * Rx
    fp_mat4_mul(out, &rz, &temp);    // out = Rz * temp
}

/* ========== VIEW MATRICES ========== */

void fp_mat4_lookat(Mat4* out,
                    float eye_x, float eye_y, float eye_z,
                    float target_x, float target_y, float target_z,
                    float up_x, float up_y, float up_z) {
    // Calculate forward vector (target - eye)
    float fx = target_x - eye_x;
    float fy = target_y - eye_y;
    float fz = target_z - eye_z;
    float f_len = sqrtf(fx*fx + fy*fy + fz*fz);
    if (f_len < 1e-8f) {
        fp_mat4_identity(out);
        return;
    }
    fx /= f_len;
    fy /= f_len;
    fz /= f_len;

    // Calculate right vector (forward × up)
    float rx = fy*up_z - fz*up_y;
    float ry = fz*up_x - fx*up_z;
    float rz = fx*up_y - fy*up_x;
    float r_len = sqrtf(rx*rx + ry*ry + rz*rz);
    if (r_len < 1e-8f) {
        fp_mat4_identity(out);
        return;
    }
    rx /= r_len;
    ry /= r_len;
    rz /= r_len;

    // Calculate true up vector (right × forward)
    float ux = ry*fz - rz*fy;
    float uy = rz*fx - rx*fz;
    float uz = rx*fy - ry*fx;

    // Build view matrix (inverse of camera transform)
    out->m[0] = rx;   out->m[4] = ux;   out->m[8]  = -fx;  out->m[12] = -(rx*eye_x + ry*eye_y + rz*eye_z);
    out->m[1] = ry;   out->m[5] = uy;   out->m[9]  = -fy;  out->m[13] = -(ux*eye_x + uy*eye_y + uz*eye_z);
    out->m[2] = rz;   out->m[6] = uz;   out->m[10] = -fz;  out->m[14] = -(-fx*eye_x - fy*eye_y - fz*eye_z);
    out->m[3] = 0.0f; out->m[7] = 0.0f; out->m[11] = 0.0f; out->m[15] = 1.0f;
}

/* ========== PROJECTION MATRICES ========== */

void fp_mat4_perspective(Mat4* out, float fov_radians, float aspect, float near, float far) {
    float f = 1.0f / tanf(fov_radians * 0.5f);
    float nf = 1.0f / (near - far);

    out->m[0] = f / aspect;  out->m[4] = 0.0f;  out->m[8]  = 0.0f;               out->m[12] = 0.0f;
    out->m[1] = 0.0f;        out->m[5] = f;     out->m[9]  = 0.0f;               out->m[13] = 0.0f;
    out->m[2] = 0.0f;        out->m[6] = 0.0f;  out->m[10] = (far + near) * nf;  out->m[14] = 2.0f * far * near * nf;
    out->m[3] = 0.0f;        out->m[7] = 0.0f;  out->m[11] = -1.0f;              out->m[15] = 0.0f;
}

void fp_mat4_ortho(Mat4* out, float left, float right, float bottom, float top, float near, float far) {
    float rl = 1.0f / (right - left);
    float tb = 1.0f / (top - bottom);
    float fn = 1.0f / (far - near);

    out->m[0] = 2.0f * rl;  out->m[4] = 0.0f;       out->m[8]  = 0.0f;       out->m[12] = -(right + left) * rl;
    out->m[1] = 0.0f;       out->m[5] = 2.0f * tb;  out->m[9]  = 0.0f;       out->m[13] = -(top + bottom) * tb;
    out->m[2] = 0.0f;       out->m[6] = 0.0f;       out->m[10] = -2.0f * fn; out->m[14] = -(far + near) * fn;
    out->m[3] = 0.0f;       out->m[7] = 0.0f;       out->m[11] = 0.0f;       out->m[15] = 1.0f;
}

/* ========== INVERSE ========== */

int fp_mat4_inverse(Mat4* out, const Mat4* m) {
    float inv[16];
    float det;

    // Calculate cofactors
    inv[0] = m->m[5]  * m->m[10] * m->m[15] -
             m->m[5]  * m->m[11] * m->m[14] -
             m->m[9]  * m->m[6]  * m->m[15] +
             m->m[9]  * m->m[7]  * m->m[14] +
             m->m[13] * m->m[6]  * m->m[11] -
             m->m[13] * m->m[7]  * m->m[10];

    inv[4] = -m->m[4]  * m->m[10] * m->m[15] +
              m->m[4]  * m->m[11] * m->m[14] +
              m->m[8]  * m->m[6]  * m->m[15] -
              m->m[8]  * m->m[7]  * m->m[14] -
              m->m[12] * m->m[6]  * m->m[11] +
              m->m[12] * m->m[7]  * m->m[10];

    inv[8] = m->m[4]  * m->m[9] * m->m[15] -
             m->m[4]  * m->m[11] * m->m[13] -
             m->m[8]  * m->m[5] * m->m[15] +
             m->m[8]  * m->m[7] * m->m[13] +
             m->m[12] * m->m[5] * m->m[11] -
             m->m[12] * m->m[7] * m->m[9];

    inv[12] = -m->m[4]  * m->m[9] * m->m[14] +
               m->m[4]  * m->m[10] * m->m[13] +
               m->m[8]  * m->m[5] * m->m[14] -
               m->m[8]  * m->m[6] * m->m[13] -
               m->m[12] * m->m[5] * m->m[10] +
               m->m[12] * m->m[6] * m->m[9];

    inv[1] = -m->m[1]  * m->m[10] * m->m[15] +
              m->m[1]  * m->m[11] * m->m[14] +
              m->m[9]  * m->m[2] * m->m[15] -
              m->m[9]  * m->m[3] * m->m[14] -
              m->m[13] * m->m[2] * m->m[11] +
              m->m[13] * m->m[3] * m->m[10];

    inv[5] = m->m[0]  * m->m[10] * m->m[15] -
             m->m[0]  * m->m[11] * m->m[14] -
             m->m[8]  * m->m[2] * m->m[15] +
             m->m[8]  * m->m[3] * m->m[14] +
             m->m[12] * m->m[2] * m->m[11] -
             m->m[12] * m->m[3] * m->m[10];

    inv[9] = -m->m[0]  * m->m[9] * m->m[15] +
              m->m[0]  * m->m[11] * m->m[13] +
              m->m[8]  * m->m[1] * m->m[15] -
              m->m[8]  * m->m[3] * m->m[13] -
              m->m[12] * m->m[1] * m->m[11] +
              m->m[12] * m->m[3] * m->m[9];

    inv[13] = m->m[0]  * m->m[9] * m->m[14] -
              m->m[0]  * m->m[10] * m->m[13] -
              m->m[8]  * m->m[1] * m->m[14] +
              m->m[8]  * m->m[2] * m->m[13] +
              m->m[12] * m->m[1] * m->m[10] -
              m->m[12] * m->m[2] * m->m[9];

    inv[2] = m->m[1]  * m->m[6] * m->m[15] -
             m->m[1]  * m->m[7] * m->m[14] -
             m->m[5]  * m->m[2] * m->m[15] +
             m->m[5]  * m->m[3] * m->m[14] +
             m->m[13] * m->m[2] * m->m[7] -
             m->m[13] * m->m[3] * m->m[6];

    inv[6] = -m->m[0]  * m->m[6] * m->m[15] +
              m->m[0]  * m->m[7] * m->m[14] +
              m->m[4]  * m->m[2] * m->m[15] -
              m->m[4]  * m->m[3] * m->m[14] -
              m->m[12] * m->m[2] * m->m[7] +
              m->m[12] * m->m[3] * m->m[6];

    inv[10] = m->m[0]  * m->m[5] * m->m[15] -
              m->m[0]  * m->m[7] * m->m[13] -
              m->m[4]  * m->m[1] * m->m[15] +
              m->m[4]  * m->m[3] * m->m[13] +
              m->m[12] * m->m[1] * m->m[7] -
              m->m[12] * m->m[3] * m->m[5];

    inv[14] = -m->m[0]  * m->m[5] * m->m[14] +
               m->m[0]  * m->m[6] * m->m[13] +
               m->m[4]  * m->m[1] * m->m[14] -
               m->m[4]  * m->m[2] * m->m[13] -
               m->m[12] * m->m[1] * m->m[6] +
               m->m[12] * m->m[2] * m->m[5];

    inv[3] = -m->m[1] * m->m[6] * m->m[11] +
              m->m[1] * m->m[7] * m->m[10] +
              m->m[5] * m->m[2] * m->m[11] -
              m->m[5] * m->m[3] * m->m[10] -
              m->m[9] * m->m[2] * m->m[7] +
              m->m[9] * m->m[3] * m->m[6];

    inv[7] = m->m[0] * m->m[6] * m->m[11] -
             m->m[0] * m->m[7] * m->m[10] -
             m->m[4] * m->m[2] * m->m[11] +
             m->m[4] * m->m[3] * m->m[10] +
             m->m[8] * m->m[2] * m->m[7] -
             m->m[8] * m->m[3] * m->m[6];

    inv[11] = -m->m[0] * m->m[5] * m->m[11] +
               m->m[0] * m->m[7] * m->m[9] +
               m->m[4] * m->m[1] * m->m[11] -
               m->m[4] * m->m[3] * m->m[9] -
               m->m[8] * m->m[1] * m->m[7] +
               m->m[8] * m->m[3] * m->m[5];

    inv[15] = m->m[0] * m->m[5] * m->m[10] -
              m->m[0] * m->m[6] * m->m[9] -
              m->m[4] * m->m[1] * m->m[10] +
              m->m[4] * m->m[2] * m->m[9] +
              m->m[8] * m->m[1] * m->m[6] -
              m->m[8] * m->m[2] * m->m[5];

    // Calculate determinant
    det = m->m[0] * inv[0] + m->m[1] * inv[4] + m->m[2] * inv[8] + m->m[3] * inv[12];

    if (fabsf(det) < 1e-8f) {
        return 0;  // Singular matrix, no inverse
    }

    det = 1.0f / det;

    // Multiply by 1/det
    for (int i = 0; i < 16; i++) {
        out->m[i] = inv[i] * det;
    }

    return 1;  // Success
}

