#ifndef LIBPORPOISE_SIM_GX_SHADER_HPP
#define LIBPORPOISE_SIM_GX_SHADER_HPP

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include <dolphin/types.h>

#include "simulator/sim_gx_State.hpp"

namespace SIM::GX {

class Shader {
 public:
  Shader(const SIM::GX::VertexFormat& format, 
         const std::array<GXAttrType, GX_VA_MAX_ATTR>& descriptors, 
         unsigned int tevStageUBO,
         unsigned int lightsUBO,
         unsigned int matrixMemUBO);
  ~Shader();
  // do we want to pass in uniform buffers in the constructor?

  // UBO structures should be the same between shader instances so the UBOs should be shared to save (V)RAM

  // consider not allow copy and assignment

  void Activate();
  // it will be up to GlRenderer to set the vertex attrib pointers (but this may be reconsidered later)

  std::optional<int> GetGlVertexAttrIdx(GXAttr gxAttribute);

  // Uniform setters
  void SetProjectionMatrix(const float * matrixData);
  void SetNumTexGens(u32 num);
  void SetTexGen(TexGenConfig texGen, int texGenNum);
  void SetTevTexMaps(const GXTexMapID * tevTexMapsArray);
  void SetTevStageConfigs(const TevStageConfig * tevStages);
  void SetPosTextureMatrixMem(const float * matrixMemBase);
  void SetNormalMatrixMem(const float * matrixMemBase);
  void SetLights(const Light * lights, const ColorChannel * colorChannels);
  void SetMatrixIndex(u32 idx);
  void SetNumChannels(u32 numChans);
  void SetInitialTevColors(float * tevColors);
  void SetNumTevStages(u32 stages);




 private:
  void GenerateVertexSource();
  void GenerateFragmentSource();
  void Compile();
  void SetupUniformLocations();
  void GenerateIndexableAttributeCode(GXAttr attr);

  SIM::GX::VertexFormat mFormat;
  std::array<GXAttrType, GX_VA_MAX_ATTR> mDescriptors;

  std::string mVertexSource;
  std::string mFragmentSource;
  s32 mProgram;
  unsigned int mVertexShader;
  unsigned int mFragmentShader;

  std::array<int, GX_VA_MAX_ATTR> mAttrLocations = {};

  // UBOs
  unsigned int mTevStageUniformBuffer = 0;
  unsigned int mLightsUniformBuffer = 0;
  unsigned int mMatrixMemoryUniformBuffer = 0;

  // Uniform locations (initialized on construction after shader compile)
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
};


class ShaderCache {
 public:
  ShaderCache();
  ~ShaderCache();

  std::shared_ptr<Shader> GetShader(const SIM::GX::VertexFormat& format, const std::array<GXAttrType, GX_VA_MAX_ATTR>& descriptors);
  void AddShader(const SIM::GX::VertexFormat& format, const std::array<GXAttrType, GX_VA_MAX_ATTR>& descriptors, std::shared_ptr<Shader> shaderPtr);

 private:
  u64 HashAttributes(const SIM::GX::VertexFormat& format, const std::array<GXAttrType, GX_VA_MAX_ATTR>& descriptors);

  std::unordered_map<u64, std::shared_ptr<Shader>> mShaderMap = {};
};
}


#endif
