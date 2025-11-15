#ifndef FP_MSAA_H
#define FP_MSAA_H

#include "gl_extensions.h"

typedef struct {
    GLuint fbo;
    GLuint color_rbo;
    GLuint depth_stencil_rbo;
    int width;
    int height;
    int samples;
} FP_MSAA_Framebuffer;

FP_MSAA_Framebuffer fp_msaa_create_framebuffer(int width, int height, int samples);
void fp_msaa_destroy_framebuffer(FP_MSAA_Framebuffer* msaa_fbo);
void fp_msaa_resolve_framebuffer(const FP_MSAA_Framebuffer* msaa_fbo, GLuint target_fbo);

#endif // FP_MSAA_H
