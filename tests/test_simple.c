#include "fp_core.h"
#include <stdio.h>

int main() {
    printf("Simple test started.\n");
    ALIGN16 Mat4 matrix = {{1,0,0,0, 0,1,0,0, 0,0,1,0, 10,20,30,1}};
    ALIGN16 Vec3f in = {1,2,3,0};
    ALIGN16 Vec3f out;

    fp_map_transform_vec3_f32(&in, &out, 1, &matrix);

    printf("Result: %f, %f, %f\n", out.x, out.y, out.z);

    return 0;
}
