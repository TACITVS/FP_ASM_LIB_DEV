/* ABI + correctness smoke test for the game-math kernels (vec3 / mat4 / quat).
 * Uses convention-agnostic properties (identity, involution, componentwise,
 * normalization) so it validates the ABI port without assuming row/col-major. */
#include "fp_types.h"
#include <stddef.h>
#include <stdio.h>
#include <math.h>

extern void  fp_mat4_identity(Mat4*);
extern void  fp_mat4_mul(Mat4*, const Mat4*, const Mat4*);
extern void  fp_mat4_transpose(Mat4*, const Mat4*);
extern void  fp_mat4_mul_vec3(Vec3f*, const Mat4*, const Vec3f*);
extern void  fp_mat4_mul_vec3_batch(Vec3f*, const Mat4*, const Vec3f*, int);
extern void  fp_map_transform_vec3_f32(const Vec3f*, Vec3f*, size_t, const Mat4*);
extern void  fp_zipWith_vec3_add_f32(const Vec3f*, const Vec3f*, Vec3f*, size_t);
extern void  fp_reduce_vec3_add_f32(const Vec3f*, size_t, Vec3f*);
extern float fp_fold_vec3_dot_f32(const Vec3f*, const Vec3f*, size_t);
extern void  fp_map_quat_rotate_vec3_f32(const Vec3f*, Vec3f*, size_t, const Quaternion*);
extern void  fp_quat_normalize_asm(Quaternion*, const Quaternion*);
extern void  fp_quat_to_mat4(Mat4*, const Quaternion*);

static int fails = 0;
static void ok(const char* t, int pass){ if(pass) printf("ok   %s\n",t); else {printf("FAIL %s\n",t); fails++;} }
static int feq(float a, float b){ return fabsf(a-b) <= 1e-4f*(1.0f+fabsf(b)); }
static int veq(Vec3f a, Vec3f b){ return feq(a.x,b.x)&&feq(a.y,b.y)&&feq(a.z,b.z); }

int main(void){
    Mat4 I; fp_mat4_identity(&I);
    { int d=1; for(int i=0;i<16;i++) d &= feq(I.m[i], (i%5==0)?1.0f:0.0f); ok("mat4_identity", d); }

    /* arbitrary non-symmetric matrix */
    Mat4 A; for(int i=0;i<16;i++) A.m[i]=(float)(i*i%7)-3.0f+0.25f*i;

    Mat4 AI, IA;
    fp_mat4_mul(&AI, &A, &I);
    fp_mat4_mul(&IA, &I, &A);
    { int p=1,q=1; for(int i=0;i<16;i++){p&=feq(AI.m[i],A.m[i]); q&=feq(IA.m[i],A.m[i]);} ok("mat4_mul A*I==A", p); ok("mat4_mul I*A==A", q); }

    Mat4 At, Att; fp_mat4_transpose(&At,&A); fp_mat4_transpose(&Att,&At);
    { int inv=1,sw=1; for(int i=0;i<4;i++)for(int j=0;j<4;j++){ sw&=feq(At.m[i*4+j],A.m[j*4+i]); inv&=feq(Att.m[i*4+j],A.m[i*4+j]); }
      ok("mat4_transpose swaps ij", sw); ok("mat4_transpose involution", inv); }

    Vec3f v = {1.5f,-2.0f,3.25f,0}, r;
    fp_mat4_mul_vec3(&r,&I,&v);          ok("mat4_mul_vec3 identity", veq(r,v));

    Vec3f vin[5], vout[5];
    for(int i=0;i<5;i++){ vin[i]=(Vec3f){(float)i,(float)(i*2-3),(float)(-i),0}; }
    fp_mat4_mul_vec3_batch(vout,&I,vin,5);
    { int p=1; for(int i=0;i<5;i++)p&=veq(vout[i],vin[i]); ok("mat4_mul_vec3_batch identity", p); }
    fp_map_transform_vec3_f32(vin,vout,5,&I);
    { int p=1; for(int i=0;i<5;i++)p&=veq(vout[i],vin[i]); ok("map_transform_vec3 identity", p); }

    Vec3f a[4],b[4],sum,zsum={0,0,0,0}; float dot=0;
    for(int i=0;i<4;i++){ a[i]=(Vec3f){(float)(i+1),(float)(i-1),2.0f,0}; b[i]=(Vec3f){0.5f,(float)i,-1.0f,0};
        zsum.x+=a[i].x; zsum.y+=a[i].y; zsum.z+=a[i].z; dot+=a[i].x*b[i].x+a[i].y*b[i].y+a[i].z*b[i].z; }
    Vec3f zres[4]; fp_zipWith_vec3_add_f32(a,b,zres,4);
    { int p=1; for(int i=0;i<4;i++)p&=veq(zres[i],(Vec3f){a[i].x+b[i].x,a[i].y+b[i].y,a[i].z+b[i].z,0}); ok("zipWith_vec3_add", p); }
    fp_reduce_vec3_add_f32(a,4,&sum);    ok("reduce_vec3_add", veq(sum,zsum));
    ok("fold_vec3_dot", feq(fp_fold_vec3_dot_f32(a,b,4), dot));

    /* quaternion: normalize a non-unit quat -> unit length */
    Quaternion q={1,2,3,4}, qn; fp_quat_normalize_asm(&qn,&q);
    ok("quat_normalize unit", feq(sqrtf(qn.x*qn.x+qn.y*qn.y+qn.z*qn.z+qn.w*qn.w), 1.0f));

    /* identity quaternion rotates vectors unchanged; maps to identity matrix */
    Quaternion qi={0,0,0,1};
    fp_map_quat_rotate_vec3_f32(vin,vout,5,&qi);
    { int p=1; for(int i=0;i<5;i++)p&=veq(vout[i],vin[i]); ok("quat_rotate identity", p); }
    Mat4 Q; fp_quat_to_mat4(&Q,&qi);
    { int d=1; for(int i=0;i<16;i++) d &= feq(Q.m[i], (i%5==0)?1.0f:0.0f); ok("quat_to_mat4 identity", d); }

    printf("\n%s (%d failure%s)\n", fails?"FAILED":"ALL PASS", fails, fails==1?"":"s");
    return fails?1:0;
}
