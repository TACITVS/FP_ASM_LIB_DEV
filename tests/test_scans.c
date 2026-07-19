#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <math.h>
extern void fp_scan_add_i64(const int64_t*,int64_t*,size_t);
extern void fp_scan_add_f64(const double*,double*,size_t);
int main(void){ int fails=0;
  for(size_t N=1;N<=40;N++){ int64_t xi[40],oi[40]; double xd[40],od[40];
    for(size_t i=0;i<N;i++){xi[i]=(int64_t)i-5; xd[i]=(double)i*0.5-2.0;}
    fp_scan_add_i64(xi,oi,N); { int64_t s=0; for(size_t i=0;i<N;i++){s+=xi[i]; if(oi[i]!=s){printf("i64 N=%zu@%zu got=%lld want=%lld\n",N,i,(long long)oi[i],(long long)s);fails++;break;}} }
    fp_scan_add_f64(xd,od,N); { double s=0; for(size_t i=0;i<N;i++){s+=xd[i]; if(fabs(od[i]-s)>1e-9){printf("f64 N=%zu@%zu\n",N,i);fails++;break;}} }
  }
  printf("%s (%d failures)\n", fails?"FAILED":"ALL PASS scans (prefix sum i64/f64, N=1..40)", fails);
  return fails?1:0; }
