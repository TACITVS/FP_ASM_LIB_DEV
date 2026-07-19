#include "fp_types.h"
#include <stddef.h>
#include <stdio.h>
#include <math.h>
extern void fp_moments_f64(const double*, size_t, double*);
extern void fp_descriptive_stats_f64(const double*, size_t, DescriptiveStats*);
int main(void){ int fails=0;
  double d[8]={2,4,4,4,5,5,7,9};
  double s1=0,s2=0,s3=0,s4=0; for(int i=0;i<8;i++){double x=d[i];s1+=x;s2+=x*x;s3+=x*x*x;s4+=x*x*x*x;}
  double m[4]; fp_moments_f64(d,8,m);
  if(fabs(m[0]-s1)>1e-6){printf("FAIL m1=%.4f want Sx=%.4f\n",m[0],s1);fails++;}
  double cmean=s1/8, cvar=0; for(int i=0;i<8;i++)cvar+=(d[i]-cmean)*(d[i]-cmean); cvar/=8;
  DescriptiveStats st; fp_descriptive_stats_f64(d,8,&st);
  if(fabs(st.mean-cmean)>1e-9){printf("FAIL mean\n");fails++;}
  if(fabs(st.variance-cvar)>1e-6){printf("FAIL var=%.6f want %.6f\n",st.variance,cvar);fails++;}
  if(fabs(st.std_dev-sqrt(cvar))>1e-6){printf("FAIL std\n");fails++;}
  if(!isfinite(st.skewness)||!isfinite(st.kurtosis)){printf("FAIL skew/kurt\n");fails++;}
  printf("moments m1=%.1f (Sx=%.1f)  stats mean=%.4f var=%.4f std=%.4f skew=%.4f kurt=%.4f\n",
         m[0],s1,st.mean,st.variance,st.std_dev,st.skewness,st.kurtosis);
  printf("%s (%d failures)\n", fails?"FAILED":"ALL PASS descriptive_stats (moments raw sums + composed stats via internal call)", fails);
  return fails?1:0; }
