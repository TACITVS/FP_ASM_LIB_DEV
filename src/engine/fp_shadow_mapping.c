#include "fp_shadow_mapping.h"
#include <stdio.h>

FP_ShadowMap fp_shadow_create_map() {
    FP_ShadowMap shadow_map = {0};

    // Create depth texture
    glGenTextures(1, &shadow_map.depth_map_texture);
    glBindTexture(GL_TEXTURE_2D, shadow_map.depth_map_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // Create framebuffer
    glGenFramebuffers(1, &shadow_map.depth_map_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, shadow_map.depth_map_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadow_map.depth_map_texture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "Failed to create shadow map framebuffer\n");
        fp_shadow_destroy_map(&shadow_map);
        return (FP_ShadowMap){0};
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return shadow_map;
}

void fp_shadow_destroy_map(FP_ShadowMap* shadow_map) {
    if (shadow_map) {
        glDeleteFramebuffers(1, &shadow_map->depth_map_fbo);
        glDeleteTextures(1, &shadow_map->depth_map_texture);
        *shadow_map = (FP_ShadowMap){0};
    }
}