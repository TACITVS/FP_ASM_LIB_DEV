#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "../include/fp_3d_math.h"

#define FP_TOLERANCE 1e-6f
#define MAX_TEST_ELEMS 32

static void fill_test_vectors(Vec3f* dst, size_t n, float sx, float sy, float sz) {
    for (size_t i = 0; i < n; ++i) {
        dst[i].x = sx * (float)i + 0.125f;
        dst[i].y = sy * (float)i - 0.25f;
        dst[i].z = sz * (float)i + 0.5f;
        dst[i]._padding = 0.0f;
    }
}

static void assert_vec3_close(const Vec3f* expected, const Vec3f* actual, size_t n, const char* label) {
    for (size_t i = 0; i < n; ++i) {
        const float dx = fabsf(expected[i].x - actual[i].x);
        const float dy = fabsf(expected[i].y - actual[i].y);
        const float dz = fabsf(expected[i].z - actual[i].z);
        if (dx > FP_TOLERANCE || dy > FP_TOLERANCE || dz > FP_TOLERANCE) {
            fprintf(stderr,
                "%s mismatch at index %zu -> (%.7f, %.7f, %.7f) vs (%.7f, %.7f, %.7f)\n",
                label,
                i,
                expected[i].x,
                expected[i].y,
                expected[i].z,
                actual[i].x,
                actual[i].y,
                actual[i].z);
            exit(EXIT_FAILURE);
        }
    }
}

static void test_zip_add(void) {
    const size_t counts[] = {0, 1, 7, 8, 17};
    Vec3f in_a[MAX_TEST_ELEMS];
    Vec3f in_b[MAX_TEST_ELEMS];
    Vec3f out_ref[MAX_TEST_ELEMS];
    Vec3f out_fp[MAX_TEST_ELEMS];

    fill_test_vectors(in_a, MAX_TEST_ELEMS, 0.1f, -0.05f, 0.02f);
    fill_test_vectors(in_b, MAX_TEST_ELEMS, -0.02f, 0.03f, -0.04f);

    for (size_t ci = 0; ci < sizeof(counts) / sizeof(counts[0]); ++ci) {
        const size_t n = counts[ci];
        ref_zipWith_vec3_add_f32(in_a, in_b, out_ref, n);
        fp_zipWith_vec3_add_f32(in_a, in_b, out_fp, n);
        assert_vec3_close(out_ref, out_fp, n, "fp_zipWith_vec3_add_f32");
    }
}

static void test_map_transform(void) {
    const size_t counts[] = {0, 1, 7, 8, 17};
    Vec3f input[MAX_TEST_ELEMS];
    Vec3f out_ref[MAX_TEST_ELEMS];
    Vec3f out_fp[MAX_TEST_ELEMS];

    Mat4f matrix = {{
        1.0f, 0.1f, -0.2f, 0.0f,
        0.3f, 0.9f, 0.4f, 0.0f,
        -0.5f, 0.6f, 1.1f, 0.0f,
        1.5f, -0.5f, 2.0f, 1.0f
    }};

    fill_test_vectors(input, MAX_TEST_ELEMS, 0.25f, -0.125f, 0.05f);

    for (size_t ci = 0; ci < sizeof(counts) / sizeof(counts[0]); ++ci) {
        const size_t n = counts[ci];
        ref_map_transform_vec3_f32(input, out_ref, n, &matrix);
        fp_map_transform_vec3_f32(input, out_fp, n, &matrix);
        assert_vec3_close(out_ref, out_fp, n, "fp_map_transform_vec3_f32");
    }
}

static void test_map_quat_rotate(void) {
    const size_t counts[] = {0, 1, 7, 8, 17};
    Vec3f input[MAX_TEST_ELEMS];
    Vec3f out_ref[MAX_TEST_ELEMS];
    Vec3f out_fp[MAX_TEST_ELEMS];

    QuatF32 quat = {0.0f, 0.0f, 0.70710677f, 0.70710677f};

    fill_test_vectors(input, MAX_TEST_ELEMS, 0.15f, 0.05f, -0.1f);

    for (size_t ci = 0; ci < sizeof(counts) / sizeof(counts[0]); ++ci) {
        const size_t n = counts[ci];
        ref_map_quat_rotate_vec3_f32(input, out_ref, n, &quat);
        fp_map_quat_rotate_vec3_f32(input, out_fp, n, &quat);
        assert_vec3_close(out_ref, out_fp, n, "fp_map_quat_rotate_vec3_f32");
    }
}

static void test_fold_dot(void) {
    const size_t counts[] = {0, 1, 7, 8, 17};
    Vec3f in_a[MAX_TEST_ELEMS];
    Vec3f in_b[MAX_TEST_ELEMS];

    fill_test_vectors(in_a, MAX_TEST_ELEMS, 0.12f, -0.08f, 0.03f);
    fill_test_vectors(in_b, MAX_TEST_ELEMS, -0.02f, 0.17f, -0.06f);

    for (size_t ci = 0; ci < sizeof(counts) / sizeof(counts[0]); ++ci) {
        const size_t n = counts[ci];
        const float ref = ref_fold_vec3_dot_f32(in_a, in_b, n);
        const float got = fp_fold_vec3_dot_f32(in_a, in_b, n);
        const float diff = fabsf(ref - got);
        if (diff > FP_TOLERANCE) {
            fprintf(stderr, "fp_fold_vec3_dot_f32 mismatch for n=%zu -> %.8f vs %.8f\n", n, ref, got);
            exit(EXIT_FAILURE);
        }
    }
}

int main(void) {
    printf("Running test_zip_add\n");
    test_zip_add();
    printf("Running test_map_transform\n");
    test_map_transform();
    printf("Running test_map_quat_rotate\n");
    test_map_quat_rotate();
    printf("Running test_fold_dot\n");
    test_fold_dot();
    printf("All 3D math tests passed.\n");
    return 0;
}
