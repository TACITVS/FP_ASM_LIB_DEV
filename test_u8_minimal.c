#include <stdio.h>
#include "fp_core.h"

int main() {
    uint8_t data[5] = {1, 2, 3, 4, 5};
    uint8_t sum = fp_reduce_add_u8(data, 5);
    printf("Sum: %d\n", sum);
    return 0;
}
