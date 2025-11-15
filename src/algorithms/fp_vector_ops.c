/**
 * fp_vector_ops.c - 3D Vector Operations
 *
 * Essential vector math for lighting, physics, and rendering.
 * These are called frequently in game engines for:
 * - Normal calculations (cross product)
 * - Lighting (dot product, normalize)
 * - Distance calculations (length, distance)
 * - Reflections and physics (reflect, project)
 */

#include "fp_core.h"
#include <math.h>

/* ========== BASIC VECTOR OPERATIONS ========== */

float vec3_dot(const Vec3f* a, const Vec3f* b) {
    return fp_fold_vec3_dot_f32(a, b, 1);
}

float vec3_length(const Vec3f* v) {
    return sqrtf(v->x * v->x + v->y * v->y + v->z * v->z);
}

float vec3_length_squared(const Vec3f* v) {
    return v->x * v->x + v->y * v->y + v->z * v->z;
}

float vec3_distance(const Vec3f* a, const Vec3f* b) {
    float dx = b->x - a->x;
    float dy = b->y - a->y;
    float dz = b->z - a->z;
    return sqrtf(dx*dx + dy*dy + dz*dz);
}

void vec3_normalize(Vec3f* out, const Vec3f* v) {
    float len = vec3_length(v);
    if (len < 1e-8f) {
        out->x = 0.0f;
        out->y = 0.0f;
        out->z = 0.0f;
        out->_pad = 0.0f;
        return;
    }
    float inv_len = 1.0f / len;
    out->x = v->x * inv_len;
    out->y = v->y * inv_len;
    out->z = v->z * inv_len;
    out->_pad = 0.0f;
}

void vec3_cross(Vec3f* out, const Vec3f* a, const Vec3f* b) {
    // Compute cross product: a × b
    // Result is perpendicular to both a and b
    float x = a->y * b->z - a->z * b->y;
    float y = a->z * b->x - a->x * b->z;
    float z = a->x * b->y - a->y * b->x;

    out->x = x;
    out->y = y;
    out->z = z;
    out->_pad = 0.0f;
}

/* ========== VECTOR ARITHMETIC ========== */

void vec3_add(Vec3f* out, const Vec3f* a, const Vec3f* b) {
    fp_zipWith_vec3_add_f32(a, b, out, 1);
}

void vec3_sub(Vec3f* out, const Vec3f* a, const Vec3f* b) {
    out->x = a->x - b->x;
    out->y = a->y - b->y;
    out->z = a->z - b->z;
    out->_pad = 0.0f;
}

void vec3_scale(Vec3f* out, const Vec3f* v, float s) {
    out->x = v->x * s;
    out->y = v->y * s;
    out->z = v->z * s;
    out->_pad = 0.0f;
}

void vec3_lerp(Vec3f* out, const Vec3f* a, const Vec3f* b, float t) {
    // Linear interpolation: out = a + t * (b - a) = (1-t)*a + t*b
    out->x = a->x + t * (b->x - a->x);
    out->y = a->y + t * (b->y - a->y);
    out->z = a->z + t * (b->z - a->z);
    out->_pad = 0.0f;
}

void vec3_mul_comp(Vec3f* out, const Vec3f* a, const Vec3f* b) {
    out->x = a->x * b->x;
    out->y = a->y * b->y;
    out->z = a->z * b->z;
    out->_pad = 0.0f;
}

void vec3_clamp(Vec3f* out, const Vec3f* v, float min, float max) {
    out->x = fmaxf(min, fminf(max, v->x));
    out->y = fmaxf(min, fminf(max, v->y));
    out->z = fmaxf(min, fminf(max, v->z));
    out->_pad = 0.0f;
}

/* ========== ADVANCED OPERATIONS ========== */

void vec3_reflect(Vec3f* out, const Vec3f* incident, const Vec3f* normal) {
    // Reflection: R = I - 2 * (N · I) * N
    // incident: incoming direction
    // normal: surface normal (must be normalized)
    float dot = vec3_dot(incident, normal);
    out->x = incident->x - 2.0f * dot * normal->x;
    out->y = incident->y - 2.0f * dot * normal->y;
    out->z = incident->z - 2.0f * dot * normal->z;
    out->_pad = 0.0f;
}

void vec3_project(Vec3f* out, const Vec3f* v, const Vec3f* onto) {
    // Project v onto onto: proj = ((v · onto) / (onto · onto)) * onto
    float dot_v_onto = vec3_dot(v, onto);
    float dot_onto_onto = vec3_dot(onto, onto);

    if (dot_onto_onto < 1e-8f) {
        out->x = 0.0f;
        out->y = 0.0f;
        out->z = 0.0f;
        out->_pad = 0.0f;
        return;
    }

    float scale = dot_v_onto / dot_onto_onto;
    out->x = onto->x * scale;
    out->y = onto->y * scale;
    out->z = onto->z * scale;
    out->_pad = 0.0f;
}

/* ========== BATCHED OPERATIONS (for performance) ========== */

void vec3_normalize_batch(Vec3f* output, const Vec3f* input, int count) {
    for (int i = 0; i < count; i++) {
        vec3_normalize(&output[i], &input[i]);
    }
}

void vec3_dot_batch(float* output, const Vec3f* a, const Vec3f* b, int count) {
    for (int i = 0; i < count; i++) {
        output[i] = vec3_dot(&a[i], &b[i]);
    }
}

/* ========== LIGHTING HELPERS ========== */

float vec3_compute_diffuse(const Vec3f* light_dir, const Vec3f* normal) {
    // Lambertian diffuse: max(N · L, 0)
    // Both light_dir and normal should be normalized
    float dot = vec3_dot(normal, light_dir);
    return (dot > 0.0f) ? dot : 0.0f;
}

float vec3_compute_specular(const Vec3f* light_dir, const Vec3f* view_dir,
                           const Vec3f* normal, float shininess) {
    // Blinn-Phong specular: max(N · H, 0)^shininess
    // H = normalize(L + V)
    Vec3f half_vec;
    half_vec.x = light_dir->x + view_dir->x;
    half_vec.y = light_dir->y + view_dir->y;
    half_vec.z = light_dir->z + view_dir->z;

    Vec3f h_normalized;
    vec3_normalize(&h_normalized, &half_vec);

    float dot = vec3_dot(normal, &h_normalized);
    if (dot <= 0.0f) return 0.0f;

    return powf(dot, shininess);
}

/* ========== TRIANGLE/MESH HELPERS ========== */

void vec3_compute_triangle_normal(Vec3f* out, const Vec3f* v0, const Vec3f* v1, const Vec3f* v2) {
    // Compute face normal using cross product
    // Edge vectors: e1 = v1 - v0, e2 = v2 - v0
    // Normal = e1 × e2 (normalized)
    Vec3f e1, e2;
    vec3_sub(&e1, v1, v0);
    vec3_sub(&e2, v2, v0);

    Vec3f cross;
    vec3_cross(&cross, &e1, &e2);
    vec3_normalize(out, &cross);
}

int vec3_is_facing_forward(const Vec3f* triangle_normal, const Vec3f* view_dir) {
    // Returns 1 if triangle faces the viewer (for backface culling)
    // Triangle faces forward if normal · view_dir < 0 (opposite directions)
    return vec3_dot(triangle_normal, view_dir) < 0.0f;
}
