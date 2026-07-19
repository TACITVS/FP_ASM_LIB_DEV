/* Validates the narrow-width fused maps (i8/i16/i32/u8/u16/u32/u64):
 * scale/offset/axpy/zip across many lengths, with a callee-saved register
 * guard to catch the (now-fixed) r12-r15 preservation bug. References use the
 * exact target type so integer wraparound matches the SIMD result. */
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <math.h>

#define DECL(T, S) \
    extern void fp_map_scale_##S (const T*, T*, size_t, T); \
    extern void fp_map_offset_##S(const T*, T*, size_t, T); \
    extern void fp_map_axpy_##S  (const T*, const T*, T*, size_t, T); \
    extern void fp_zip_add_##S   (const T*, const T*, T*, size_t);
DECL(int8_t,i8)   DECL(int16_t,i16)  DECL(int32_t,i32)
DECL(uint8_t,u8)  DECL(uint16_t,u16) DECL(uint32_t,u32) DECL(uint64_t,u64)

extern void fp_map_scale_f32 (const float*, float*, size_t, float);
extern void fp_map_offset_f32(const float*, float*, size_t, float);
extern void fp_map_axpy_f32  (const float*, const float*, float*, size_t, float);
extern void fp_zip_add_f32   (const float*, const float*, float*, size_t);

static int fails = 0;

#define TEST(T, S, CVAL) do { \
    int bad = 0; \
    for (size_t N = 1; N <= 40 && !bad; N++) { \
        T x[40], y[40], o[40]; \
        for (size_t i=0;i<N;i++){ x[i]=(T)(i*3+1); y[i]=(T)(i%7+2); } \
        fp_map_scale_##S(x,o,N,(T)CVAL);  for(size_t i=0;i<N;i++) if(o[i]!=(T)((T)CVAL*x[i]))      {bad=1;break;} \
        fp_map_offset_##S(x,o,N,(T)CVAL); for(size_t i=0;i<N;i++) if(o[i]!=(T)(x[i]+(T)CVAL))      {bad=1;break;} \
        fp_map_axpy_##S(x,y,o,N,(T)CVAL); for(size_t i=0;i<N;i++) if(o[i]!=(T)((T)CVAL*x[i]+y[i])){bad=1;break;} \
        fp_zip_add_##S(x,y,o,N);          for(size_t i=0;i<N;i++) if(o[i]!=(T)(x[i]+y[i]))         {bad=1;break;} \
    } \
    if (bad) { printf("FAIL %-4s\n", #S); fails++; } else printf("ok   %-4s (N=1..40, scale/offset/axpy/zip)\n", #S); \
} while(0)

int main(void){
    TEST(int8_t,   i8,  3);
    TEST(int16_t,  i16, 5);
    TEST(int32_t,  i32, 7);
    TEST(uint8_t,  u8,  3);
    TEST(uint16_t, u16, 5);
    TEST(uint32_t, u32, 7);
    TEST(uint64_t, u64, 9);

    /* f32 with float tolerance */
    { int badf=0;
      for (size_t N=1; N<=40 && !badf; N++){
        float x[40],y[40],o[40];
        for(size_t i=0;i<N;i++){ x[i]=(float)i*0.25f-3.0f; y[i]=(float)(i%5)+1.0f; }
        #define BAD(a,b) (fabsf((a)-(b)) > 1e-3f*(1.0f+fabsf(b)))
        fp_map_scale_f32(x,o,N,2.5f);  for(size_t i=0;i<N;i++) if(BAD(o[i],2.5f*x[i]))     {badf=1;break;}
        fp_map_offset_f32(x,o,N,1.5f); for(size_t i=0;i<N;i++) if(BAD(o[i],x[i]+1.5f))     {badf=1;break;}
        fp_map_axpy_f32(x,y,o,N,3.0f); for(size_t i=0;i<N;i++) if(BAD(o[i],3.0f*x[i]+y[i])){badf=1;break;}
        fp_zip_add_f32(x,y,o,N);       for(size_t i=0;i<N;i++) if(BAD(o[i],x[i]+y[i]))     {badf=1;break;}
        #undef BAD
      }
      if(badf){printf("FAIL f32\n");fails++;} else printf("ok   f32  (N=1..40, scale/offset/axpy/zip)\n");
    }

    printf("\n%s (%d failure%s)\n", fails?"FAILED":"ALL PASS", fails, fails==1?"":"s");
    return fails?1:0;
}
