#ifndef LIBPORPOISE_SIM_GX_FRAMEBUFFER_MANAGER_HPP
#define LIBPORPOISE_SIM_GX_FRAMEBUFFER_MANAGER_HPP

#include <dolphin/types.h>
#include <memory>

namespace SIM::GX {

class EmbeddedFramebuffer {
 public:
  EmbeddedFramebuffer();
  ~EmbeddedFramebuffer();
  void SetInternalRes(u32 internalRes);
  void Activate();
  inline unsigned int GetTexture() { return mTextureId; };

 private:
  u32 mInternalRes;
  unsigned int mFboId;
  unsigned int mRboId;
  unsigned int mTextureId;
};

class FramebufferManager {
 public:
  FramebufferManager();
  ~FramebufferManager();
  static FramebufferManager& GetInstance();
  void Init();
  inline std::shared_ptr<EmbeddedFramebuffer> GetEfb() { return mEfb; };


 private:
  std::shared_ptr<EmbeddedFramebuffer> mEfb = nullptr;
};

}


#endif
