#ifndef LIBPORPOISE_SIM_GX_FRAMEBUFFER_MANAGER_HPP
#define LIBPORPOISE_SIM_GX_FRAMEBUFFER_MANAGER_HPP

#include <dolphin/types.h>
#include <memory>
#include <unordered_map>

namespace SIM::GX {

class FramebufferTexture {
 public:
  FramebufferTexture(u32 width, u32 height);
  virtual ~FramebufferTexture();
  inline u32 GetInternalRes() const {return mInternalRes;};
  virtual void SetInternalRes(u32 internalRes);
  inline unsigned int GetTexture() const { return mTextureId; };
  inline u32 GetInternalRes() { return mInternalRes; };
  void CopyFrom(const FramebufferTexture& source, u16 xOffset, u16 yOffset, u16 width, u16 height);
  
 protected:
  u32 mWidth;
  u32 mHeight;
  u32 mInternalRes;
  unsigned int mTextureId;
};

class EmbeddedFramebuffer : public FramebufferTexture{
 public:
  EmbeddedFramebuffer();
  ~EmbeddedFramebuffer();
  void Activate();
  virtual void SetInternalRes(u32 internalRes);

 private:
  unsigned int mFboId;
  unsigned int mRboId;
};

class FramebufferManager {
 public:
  FramebufferManager();
  ~FramebufferManager();
  static FramebufferManager& GetInstance();
  void Init();
  inline std::shared_ptr<EmbeddedFramebuffer> GetEfb() { return mEfb; };
  std::shared_ptr<FramebufferTexture> GetTexture(void * address);
  void AddTexture(void * address, std::shared_ptr<FramebufferTexture> texturePtr);
  inline unsigned int GetCopyFbo() { return mCopyFbo; };


 private:
  std::shared_ptr<EmbeddedFramebuffer> mEfb = nullptr;
  std::unordered_map<void *, std::shared_ptr<FramebufferTexture>> mFramebufTextureMap = {};
  unsigned int mCopyFbo = 0;
};

}


#endif
