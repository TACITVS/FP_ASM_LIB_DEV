#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "../include/fp.h"

/* Simple user functions (hot loops call these): */
static int64_t square_i64(int64_t x, void* ctx) {
    (void)ctx;
    return x * x;
}

static int64_t add_i64(int64_t acc, int64_t value, void* ctx) {
    (void)ctx;
    return acc + value;
}

int main(void) {
    int64_t in[]  = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    int64_t out[10];
    const size_t n = sizeof in / sizeof in[0];

    fp_map_apply_i64(in, out, n, square_i64, NULL);
    size_t mapped = n;
    printf("mapped = %zu\nout: ", mapped);
    for (size_t i = 0; i < n; ++i) printf("%lld%s", (long long)out[i], (i+1<n?", ":"\n"));

    int64_t sum = fp_fold_left_i64(out, n, 0, add_i64, NULL);
    printf("sum(squares) = %lld\n", (long long)sum);
    return 0;
}
