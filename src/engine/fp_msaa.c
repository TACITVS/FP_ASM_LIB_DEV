#include "fp_msaa.h"
#include <stdio.h>

FP_MSAA_Framebuffer fp_msaa_create_framebuffer(int width, int height, int samples) {
    FP_MSAA_Framebuffer fbo = {0};
    fbo.width = width;
    fbo.height = height;
    fbo.samples = samples;

    // Create Framebuffer Object
    glGenFramebuffers(1, &fbo.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo.fbo);

    // Create Color Renderbuffer Object
    glGenRenderbuffers(1, &fbo.color_rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, fbo.color_rbo);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_RGBA8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, fbo.color_rbo);

    // Create Depth/Stencil Renderbuffer Object
    glGenRenderbuffers(1, &fbo.depth_stencil_rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, fbo.depth_stencil_rbo);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fbo.depth_stencil_rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "Failed to create MSAA framebuffer\n");
        fp_msaa_destroy_framebuffer(&fbo); // Clean up
        return (FP_MSAA_Framebuffer){0};
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    return fbo;
}

void fp_msaa_destroy_framebuffer(FP_MSAA_Framebuffer* msaa_fbo) {
    if (msaa_fbo) {
        glDeleteFramebuffers(1, &msaa_fbo->fbo);
        glDeleteRenderbuffers(1, &msaa_fbo->color_rbo);
        glDeleteRenderbuffers(1, &msaa_fbo->depth_stencil_rbo);
        *msaa_fbo = (FP_MSAA_Framebuffer){0};
    }
}

void fp_msaa_resolve_framebuffer(const FP_MSAA_Framebuffer* msaa_fbo, GLuint target_fbo) {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, msaa_fbo->fbo);

    // If target_fbo is 0, we are blitting to the default framebuffer (the window)
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target_fbo);
    
    glBlitFramebuffer(0, 0, msaa_fbo->width, msaa_fbo->height, 0, 0, msaa_fbo->width, msaa_fbo->height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}
