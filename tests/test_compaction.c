/* Validates the compaction functions: filter_gt (simd/simple), take_while_gt,
 * drop_while_gt, and the 7-argument partition_gt_i64 (which stresses the ABI
 * marshalling: threshold captured in a reg, out_pass_count parked in the
 * PROLOGUE stack pad, out_fail_count read from its SysV arg7 slot). */
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

extern size_t fp_filter_gt_i64_simple(const int64_t*, int64_t*, size_t, int64_t);
extern size_t fp_filter_gt_i64_simd(const int64_t*, int64_t*, size_t, int64_t);
extern size_t fp_take_while_gt_i64(const int64_t*, int64_t*, size_t, int64_t);
extern size_t fp_drop_while_gt_i64(const int64_t*, int64_t*, size_t, int64_t);
extern void   fp_partition_gt_i64(const int64_t*, int64_t*, int64_t*, size_t,
                                  int64_t, size_t*, size_t*);

static int fails=0;
static void ok(const char* t,int p){ if(p) printf("ok   %s\n",t); else {printf("FAIL %s\n",t); fails++;} }

int main(void){
    int64_t in[10]={5,8,2,9,1,7,3,6,4,10};
    int64_t out[10];

    /* filter > 5 : {8,9,7,6,10} = 5 elements */
    size_t c = fp_filter_gt_i64_simple(in,out,10,5);
    { int good = (c==5); int64_t exp[5]={8,9,7,6,10}; for(size_t i=0;i<c&&good;i++) good&=(out[i]==exp[i]); ok("filter_gt_simple", good); }
    size_t c2 = fp_filter_gt_i64_simd(in,out,10,5);
    { int good = (c2==5); int64_t exp[5]={8,9,7,6,10}; for(size_t i=0;i<c2&&good;i++) good&=(out[i]==exp[i]); ok("filter_gt_simd", good); }

    /* take_while > 3 on {5,8,2,...}: take 5,8 then stop at 2 -> 2 elements */
    size_t t = fp_take_while_gt_i64(in,out,10,3);
    ok("take_while_gt", t==2 && out[0]==5 && out[1]==8);

    /* drop_while > 3: drop 5,8 then keep from 2 -> 8 elements starting at 2 */
    size_t d = fp_drop_while_gt_i64(in,out,10,3);
    ok("drop_while_gt", d==8 && out[0]==2 && out[1]==9);

    /* partition > 5: pass={8,9,7,6,10}(5), fail={5,2,1,3,4}(5) */
    int64_t op[10], of[10]; size_t np=999, nf=999;
    fp_partition_gt_i64(in, op, of, 10, 5, &np, &nf);
    { int good = (np==5 && nf==5);
      int64_t ep[5]={8,9,7,6,10}, ef[5]={5,2,1,3,4};
      for(size_t i=0;i<np&&good;i++) good&=(op[i]==ep[i]);
      for(size_t i=0;i<nf&&good;i++) good&=(of[i]==ef[i]);
      ok("partition_gt (7-arg)", good);
      if(!good) printf("     np=%zu nf=%zu pass[0..]=%lld,%lld fail[0..]=%lld,%lld\n",
                       np,nf,(long long)op[0],(long long)op[1],(long long)of[0],(long long)of[1]); }

    printf("\n%s (%d failure%s)\n", fails?"FAILED":"ALL PASS", fails, fails==1?"":"s");
    return fails?1:0;
}
