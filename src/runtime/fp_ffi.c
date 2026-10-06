/*
 * fp_ffi.c — runtime self-description for foreign-function bindings
 * (see include/fp_ffi.h). Compiled for the x86-64 baseline, like the rest
 * of src/runtime/, so a binding can call it before anything else.
 */
#include "fp_ffi.h"

uint32_t fp_ffi_abi_version(void) { return FP_FFI_ABI_VERSION; }

uint32_t fp_ffi_type_size(int32_t type) {
    switch (type) {
    case FP_FFI_TYPE_VEC3F:           return (uint32_t)sizeof(Vec3f);
    case FP_FFI_TYPE_QUATERNION:      return (uint32_t)sizeof(Quaternion);
    case FP_FFI_TYPE_MAT4:            return (uint32_t)sizeof(Mat4);
    case FP_FFI_TYPE_GFX_CONVENTIONS: return (uint32_t)sizeof(fp_gfx_conventions);
    default:                          return 0;
    }
}
