#ifndef LIBPORPOISE_SIM_GX_GLRENDERER_HPP
#define LIBPORPOISE_SIM_GX_GLRENDERER_HPP

#include <vector>

#include <dolphin/types.h>
#include <dolphin/gx/GXEnum.h>

namespace SIM::GX {

struct RenderVertex;

class GlRenderer {
 public:
  void Draw(const RenderVertex * vertices, size_t numVertices, GXPrimitive primitive);
  void FlushRenderVerts();

  inline int GetBatchableDrawcalls() {return mBatchableDrawcalls; };
  inline void ResetBatchableDrawcalls() {mBatchableDrawcalls = 0; };

  inline int GetTotalDrawcalls() {return mTotalDrawcalls; };
  inline void ResetTotalDrawcalls() {mTotalDrawcalls = 0; };

 private:
  void Initialize();
  void ReserveRenderVerts(int numAdditionalVerts);
  void ExpandQuads(const SIM::GX::RenderVertex * vertices, size_t numVertices);
  void ExpandQuadStrip(const SIM::GX::RenderVertex * vertices, size_t numVertices);

  unsigned int mVertexArray = 0;
  unsigned int mVertexBuffer = 0;
  unsigned int mTevStageUniformBuffer = 0;
  unsigned int mLightsUniformBuffer = 0;
  unsigned int mMatrixMemoryUniformBuffer = 0;

  RenderVertex * mRenderVerts = nullptr;
  int mRenderVertsPrimitive;
  size_t mRenderVertsCount = 0;
  size_t mRenderVertsCapacity = 0;

  // Shader Uniform locations
  int mProjectionLocation;
  int mNormalMtxLocation;
  int mNumTexGenLocation;
  int mTexGenMatrixLocation[GX_MAX_TEXCOORD];
  int mTexGenTypeLocation[GX_MAX_TEXCOORD];
  int mTevTexMapLocation;
  int mTevStageConfigsBlock;
  int mTevStageConfigsBinding;
  int mLightConfigBlock;
  int mLightConfigBlockBinding;
  int mMatrixMemoryBlock;
  int mMatrixMemoryBlockBinding;
  int mInitialTevColorsLocation;
  int mNumTevStagesLocation;
  int mNumChansLocation;
  int mMtxIdxALocation;
  int mPnMtxIdxEnabledLocation;
  size_t mVboCapacity = 0;


  int mBatchableDrawcalls = 0;
  int mTotalDrawcalls = 0;
};

GlRenderer& GetGlRenderer();

}

#endif
