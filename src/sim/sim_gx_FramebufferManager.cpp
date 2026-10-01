#include "simulator/sim_gx_FramebufferManager.hpp"

#include <simulator/glad/glad.h>

namespace SIM::GX {

static FramebufferManager sFramebufferManager;

EmbeddedFramebuffer::EmbeddedFramebuffer() {
    glGenFramebuffers(1, &mFboId);
    glGenRenderbuffers(1, &mRboId);
    glGenTextures(1, &mTextureId);
    SetInternalRes(1);
    glBindFramebuffer(GL_FRAMEBUFFER, mFboId);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mRboId);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mTextureId, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

EmbeddedFramebuffer::~EmbeddedFramebuffer() {
}

void EmbeddedFramebuffer::SetInternalRes(u32 internalRes) {
    glBindTexture(GL_TEXTURE_2D, mTextureId);
    glTexImage2D(
        GL_TEXTURE_2D, 
        0, 
        GL_RGBA, 
        640 * internalRes, 
        480 * internalRes, 
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        nullptr
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindRenderbuffer(GL_RENDERBUFFER, mRboId);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, 640 *internalRes, 480 * internalRes);
    mInternalRes = internalRes;
}

void EmbeddedFramebuffer::Activate() {
    glBindFramebuffer(GL_FRAMEBUFFER, mFboId);
    glBindRenderbuffer(GL_RENDERBUFFER, mRboId);
    GLenum drawBufs[] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, drawBufs);
}

FramebufferManager::FramebufferManager() {
}

FramebufferManager::~FramebufferManager() {}

void FramebufferManager::Init() {
    mEfb = std::make_shared<EmbeddedFramebuffer>();
}

FramebufferManager& FramebufferManager::GetInstance() {
    return sFramebufferManager;
}

}