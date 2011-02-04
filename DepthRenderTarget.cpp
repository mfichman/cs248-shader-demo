#include "DepthRenderTarget.h"

DepthRenderTarget::DepthRenderTarget(unsigned int width, unsigned int height) {
    width_ = width;
    height_ = height;

    glPushAttrib(GL_ALL_ATTRIB_BITS );

    // Initialize the texture, including filtering options
    glGenTextures(1, &textureID_);
    glBindTexture(GL_TEXTURE_2D, textureID_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(
        GL_TEXTURE_2D, 
        0, 
        GL_DEPTH_COMPONENT, 
        width_, 
        height_, 
        0, 
        GL_DEPTH_COMPONENT, 
        GL_UNSIGNED_BYTE, 
        0);

    // Generate a framebuffer
    glGenFramebuffers(1, &frameBufferID_);
    glBindFramebuffer(GL_FRAMEBUFFER, frameBufferID_);

    // Attach the texture to the frame buffer
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_2D,
        textureID_,
        0);

    // Check the status of the FBO
    glDrawBuffer(GL_NONE);
    if (GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
        std::cerr << "Invalid framebuffer configuration" << std::endl;
        exit(-1);
    }
}

DepthRenderTarget::~DepthRenderTarget() {
    glDeleteFramebuffers(1, &frameBufferID_);
    glDeleteRenderbuffers(1, &depthBufferID_);
    glDeleteTextures(1, &textureID_);
}

GLuint DepthRenderTarget::textureID() const {
    return textureID_;
}

void DepthRenderTarget::bind() {
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glBindFramebuffer(GL_FRAMEBUFFER, frameBufferID_);
    glViewport(0, 0, width_, height_);
    glDrawBuffer(GL_NONE);
}

void DepthRenderTarget::unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glPopAttrib();
}