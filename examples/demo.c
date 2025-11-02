#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "../include/fp.h"

/* Simple user functions (hot loops call these): */
static int64_t square_i64(int64_t x) { return x * x; }
static int64_t add_i64(int64_t a, int64_t b) { return a + b; }

int main(void) {
    int64_t in[]  = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    int64_t out[10];
    const size_t n = sizeof in / sizeof in[0];

    size_t mapped = fp_map_i64(in, out, n, square_i64);
    printf("mapped = %zu\nout: ", mapped);
    for (size_t i = 0; i < n; ++i) printf("%lld%s", (long long)out[i], (i+1<n?", ":"\n"));

    int64_t sum = fp_reduce_i64(out, n, 0, add_i64);
    printf("sum(squares) = %lld\n", (long long)sum);
    return 0;
}
