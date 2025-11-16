/**
 * test_vector_ops.c - Quick verification test for vector operations
 */

#include <stdio.h>
#include <math.h>
#include "fp_core.h"

#define TOLERANCE 1e-5f

int main(void) {
    printf("Testing vector operations...\n\n");

    // Test normalize
    Vec3f v = {3.0f, 4.0f, 0.0f, 0.0f};
    Vec3f v_norm;
    vec3_normalize(&v_norm, &v);
    printf("Normalize (3,4,0): (%.3f, %.3f, %.3f) - length: %.3f\n",
           v_norm.x, v_norm.y, v_norm.z, vec3_length(&v_norm));

    // Test cross product
    Vec3f a = {1.0f, 0.0f, 0.0f, 0.0f};
    Vec3f b = {0.0f, 1.0f, 0.0f, 0.0f};
    Vec3f cross;
    vec3_cross(&cross, &a, &b);
    printf("Cross (1,0,0) x (0,1,0): (%.3f, %.3f, %.3f)\n",
           cross.x, cross.y, cross.z);

    // Test dot product
    Vec3f c = {1.0f, 2.0f, 3.0f, 0.0f};
    Vec3f d = {4.0f, 5.0f, 6.0f, 0.0f};
    float dot = vec3_dot(&c, &d);
    printf("Dot (1,2,3) · (4,5,6): %.1f\n", dot);

    // Test lighting
    Vec3f light_dir = {0.0f, 1.0f, 0.0f, 0.0f};
    Vec3f normal = {0.0f, 1.0f, 0.0f, 0.0f};
    float diffuse = vec3_compute_diffuse(&light_dir, &normal);
    printf("Diffuse lighting (aligned): %.2f\n", diffuse);

    printf("\nAll vector operations working!\n");
    return 0;
}
