#include "simulator/sim_gx_FramebufferManager.hpp"

#include <simulator/glad/glad.h>

namespace SIM::GX {

static FramebufferManager sFramebufferManager;

FramebufferTexture::FramebufferTexture(u32 width, u32 height) :
    mWidth(width),
    mHeight(height)
{
    glGenTextures(1, &mTextureId);
    SetInternalRes(1);
}

FramebufferTexture::~FramebufferTexture() {
    glDeleteTextures(1, &mTextureId);
}

void FramebufferTexture::SetInternalRes(u32 internalRes) {
    glBindTexture(GL_TEXTURE_2D, mTextureId);
    glTexImage2D(
        GL_TEXTURE_2D, 
        0, 
        GL_RGBA, 
        mWidth * internalRes, 
        mHeight * internalRes, 
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        nullptr
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    mInternalRes = internalRes;
}

void FramebufferTexture::CopyFrom(const FramebufferTexture& source, u16 xOffset, u16 yOffset, u16 width, u16 height) {
    auto& fbMan = FramebufferManager::GetInstance();

    int drawFbo = 0;
    int readFbo = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFbo);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFbo);

    glBindFramebuffer(GL_FRAMEBUFFER, fbMan.GetCopyFbo());

    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, source.GetTexture(), 0);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, mTextureId, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT1);
    glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, drawFbo);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, readFbo);
}

EmbeddedFramebuffer::EmbeddedFramebuffer() : FramebufferTexture(640, 480) {
    glGenFramebuffers(1, &mFboId);
    glGenRenderbuffers(1, &mRboId);
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
    FramebufferTexture::SetInternalRes(internalRes);
    glBindFramebuffer(GL_FRAMEBUFFER, mFboId);
    glBindRenderbuffer(GL_RENDERBUFFER, mRboId);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, mWidth *internalRes, mHeight * internalRes);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mRboId);
}


void EmbeddedFramebuffer::Activate() {
    glBindFramebuffer(GL_FRAMEBUFFER, mFboId);
    glBindRenderbuffer(GL_RENDERBUFFER, mRboId);
    GLenum drawBufs[] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, drawBufs);
    glViewport(0, 0, 640, 480);
}

FramebufferManager::FramebufferManager() {
}

FramebufferManager::~FramebufferManager() {}

void FramebufferManager::Init() {
    mEfb = std::make_shared<EmbeddedFramebuffer>();
    glGenFramebuffers(1, &mCopyFbo);
}

std::shared_ptr<FramebufferTexture> FramebufferManager::GetTexture(void * address) {
    if(mFramebufTextureMap.count(address)) {
        return mFramebufTextureMap[address];
    }

    return nullptr;
}

void FramebufferManager::AddTexture(void * address, std::shared_ptr<FramebufferTexture> texturePtr) {
    if(mFramebufTextureMap.count(address)) {
        mFramebufTextureMap.erase(address);
    }

    mFramebufTextureMap[address] = texturePtr;
}

FramebufferManager& FramebufferManager::GetInstance() {
    return sFramebufferManager;
}

}