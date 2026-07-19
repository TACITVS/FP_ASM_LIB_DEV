/* Validates tier2 set ops: unique (3-arg), union and intersect (5-arg, len_b
 * on Win64 stack / SysV r8). Inputs are sorted (typical requirement for these
 * merge-based set operations). */
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

extern size_t fp_unique_i64(const int64_t*, int64_t*, size_t);
extern size_t fp_union_i64(const int64_t*, const int64_t*, int64_t*, size_t, size_t);
extern size_t fp_intersect_i64(const int64_t*, const int64_t*, int64_t*, size_t, size_t);

static int fails=0;
static void ok(const char* t,int p){ if(p) printf("ok   %s\n",t); else {printf("FAIL %s\n",t); fails++;} }

int main(void){
    int64_t out[16];

    int64_t u[7]={1,1,2,3,3,3,4};
    size_t nu = fp_unique_i64(u,out,7);
    { int good=(nu==4); int64_t e[4]={1,2,3,4}; for(size_t i=0;i<nu&&good;i++) good&=(out[i]==e[i]);
      ok("unique", good); if(!good) printf("     nu=%zu\n",nu); }

    int64_t a[4]={1,2,3,4}, b[4]={3,4,5,6};
    size_t nun = fp_union_i64(a,b,out,4,4);
    { int good=(nun==6); int64_t e[6]={1,2,3,4,5,6}; for(size_t i=0;i<nun&&good;i++) good&=(out[i]==e[i]);
      ok("union (5-arg)", good); if(!good) printf("     nun=%zu\n",nun); }

    size_t ni = fp_intersect_i64(a,b,out,4,4);
    { int good=(ni==2); int64_t e[2]={3,4}; for(size_t i=0;i<ni&&good;i++) good&=(out[i]==e[i]);
      ok("intersect (5-arg)", good); if(!good) printf("     ni=%zu out=%lld,%lld\n",ni,(long long)out[0],(long long)out[1]); }

    printf("\n%s (%d failure%s)\n", fails?"FAILED":"ALL PASS", fails, fails==1?"":"s");
    return fails?1:0;
}
