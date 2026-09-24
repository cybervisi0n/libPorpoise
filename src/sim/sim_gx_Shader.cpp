#include "simulator/sim_gx_Shader.hpp"

#include "simulator/glad/glad.h"
#include "simulator/sim_crc32.h"

#include <dolphin.h>

#include <format>
#include <SDL2/SDL.h>


const char * SIM_GXVertexShader = 
#include "shaders/vertex.glsl"
;

const char * SIM_GXFragmentShader = 
#include "shaders/fragment.glsl"
;

static constexpr std::array<const char *, GX_VA_MAX_ATTR> VertexAttributeStrings = {
    "posNormalMtxIdx",

    // Note: these are combined into two uvec4 vertex attributes to save on attributes
    "tex0MtxIdx",
    "tex1MtxIdx",
    "tex2MtxIdx",
    "tex3MtxIdx",
    "tex4MtxIdx",
    "tex5MtxIdx",
    "tex6MtxIdx",
    "tex7MtxIdx",

    "position",
    "normal",
    "color0",
    "color1",

    // note: these might get combined into only 4 attributes to save on attribute space
    "texCoord0",
    "texCoord1",
    "texCoord2",
    "texCoord3",
    "texCoord4",
    "texCoord5",
    "texCoord6",
    "texCoord7",

    "posMtxArray",
    "normalMtxArray",
    "texMtxArray",
    "lightArray",
    "nbt"
};

static std::string GetTypeName(GXAttrType descriptor, GXCompType type, GXCompCnt cnt) {
    if(descriptor == GX_INDEX8 || descriptor == GX_INDEX16) {
        return "uint";
    }

    

    return "notImplemented";
}

static bool CompileShader(GLuint& id, const char * source) {
    glShaderSource( id, 1, &source, NULL );
    glCompileShader( id );
    GLint status;
    glGetShaderiv(id, GL_COMPILE_STATUS, &status);
    if( status != GL_TRUE )
    {
        GLint logSize;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &logSize);
        GLchar * errorBuf = new GLchar[logSize];
        glGetShaderInfoLog(id, logSize, &logSize, (GLchar *)errorBuf);
        std::string errorMessageString = "Error compiling shader:\n" + std::string((char*)errorBuf);
        SDL_ShowSimpleMessageBox(0, "Error compiling shader", errorMessageString.c_str(), nullptr);
        delete[] errorBuf;
        return false;
    }
    return true;
}

static bool LinkShader(GLuint id,GLuint vertex,GLuint fragment) {
    glAttachShader( id, vertex );
    glAttachShader( id, fragment );
    glLinkProgram( id );
    GLint status;
    GLint logSize;
    glGetProgramiv( id, GL_LINK_STATUS, &status );
    if( status != GL_TRUE )
    {
        glGetProgramiv(id, GL_INFO_LOG_LENGTH, &logSize);
        GLchar * errorBuf = new GLchar[logSize];
        glGetProgramInfoLog(id, logSize, &logSize, (GLchar *)errorBuf);
        std::string errorMessageString = "Error linking shader:\n" + std::string((char*)errorBuf);
        SDL_ShowSimpleMessageBox(0, "Error linking shader", errorMessageString.c_str(), nullptr);
        delete[] errorBuf;
        return false;
    }
    glValidateProgram(id);
    glUseProgram(id);

    //glGenVertexArrays(1, &gxVertexArray);
    //glBindVertexArray(gxVertexArray);
    //glGenBuffers(1, &gxVertexBuffer);
    return true;
}



namespace SIM::GX {

Shader::Shader(const SIM::GX::VertexFormat& format, 
               const std::array<GXAttrType, GX_VA_MAX_ATTR>& descriptors,
               unsigned int tevStageUBO,
               unsigned int lightsUBO,
               unsigned int matrixMemUBO) :
    mFormat(format),
    mDescriptors(descriptors),
    mTevStageUniformBuffer(tevStageUBO),
    mLightsUniformBuffer(lightsUBO),
    mMatrixMemoryUniformBuffer(matrixMemUBO)
{
    // Generate shader source and compile
    GenerateVertexSource();
    GenerateFragmentSource();

    Compile();
    SetupUniformLocations();
}

Shader::~Shader() {}

// Uniform Setters
void Shader::SetProjectionMatrix(const float * matrixData) {
    glUniformMatrix4fv(
        mProjectionLocation,
        1,
        GL_TRUE,
        matrixData);
}

void Shader::SetNumTexGens(u32 num) {
    glUniform1ui(mNumTexGenLocation, num);
}

void Shader::SetTexGen(TexGenConfig texGen, int texGenNum) {
    glUniform1ui(mTexGenMatrixLocation[texGenNum], texGen.mMatrixId);
    glUniform1ui(mTexGenTypeLocation[texGenNum], texGen.mType);
}

void Shader::SetTevTexMaps(const GXTexMapID * tevTexMapsArray) {
    glUniform1iv(mTevTexMapLocation, GX_MAX_TEVSTAGE, (const GLint*)tevTexMapsArray);
}

void Shader::SetTevStageConfigs(const TevStageConfig * tevStages) {
    glBindBuffer(GL_UNIFORM_BUFFER, mTevStageUniformBuffer);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(TevStageConfig) * GX_MAX_TEVSTAGE, tevStages);
    glBindBufferBase(GL_UNIFORM_BUFFER, mTevStageConfigsBinding, mTevStageUniformBuffer);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void Shader::SetPosTextureMatrixMem(const float * matrixMemBase) {
    glBindBuffer(GL_UNIFORM_BUFFER, mMatrixMemoryUniformBuffer);
    //const float * matrixMem = gxState.GetXfMemoryPointer();
    glBufferSubData(GL_UNIFORM_BUFFER, 0, 240 * sizeof(float), matrixMemBase);
    glBindBufferBase(GL_UNIFORM_BUFFER, mMatrixMemoryBlockBinding, mMatrixMemoryUniformBuffer);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void Shader::SetNormalMatrixMem(const float * matrixMemBase) {
    glBindBuffer(GL_UNIFORM_BUFFER, mMatrixMemoryUniformBuffer);
    const float * matrixMem = matrixMemBase + 0x400;
    float normalMatrices[30][4] = {};
    for(int i=0; i < 30; i++) {
        normalMatrices[i][0] = matrixMem[(3*i)];
        normalMatrices[i][1] = matrixMem[(3*i) + 1];
        normalMatrices[i][2] = matrixMem[(3*i) + 2];
        normalMatrices[i][3] = 0.0f;
    }
    glBufferSubData(GL_UNIFORM_BUFFER, 240 * sizeof(float), sizeof(normalMatrices), normalMatrices);
    //int glError = glGetError();
    //if(glError != GL_NO_ERROR) {
    //    printf("Error sending normal matrix data %d\n", glError);
    //}
    glBindBufferBase(GL_UNIFORM_BUFFER, mMatrixMemoryBlockBinding, mMatrixMemoryUniformBuffer);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void Shader::SetLights(const Light * lights, const ColorChannel * colorChannels) {
    glBindBuffer(GL_UNIFORM_BUFFER, mLightsUniformBuffer);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Light) * 8, lights);
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(Light) * 8, sizeof(ColorChannel) * 4, colorChannels);
    glBindBufferBase(GL_UNIFORM_BUFFER, mLightConfigBlockBinding, mLightsUniformBuffer);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void Shader::SetMatrixIndex(u32 idx) {
    glUniform1ui(mMtxIdxALocation, idx);
}

void Shader::SetNumChannels(u32 numChans) {
    glUniform1ui(mNumChansLocation, numChans);
}

void Shader::SetInitialTevColors(float * tevColors) {
    glUniform4fv(mInitialTevColorsLocation, 4, tevColors);
}

void Shader::SetNumTevStages(u32 stages) {
    glUniform1ui(mNumTevStagesLocation, stages);
}

// Activates the shader in GL
void Shader::Activate() {
    glUseProgram(mProgram);
}

void Shader::GenerateVertexSource() {
    mVertexSource = "#version 330 core \n";

    //Create vertex layout
    int currentLoc = 0;
    if(mDescriptors[GX_VA_PNMTXIDX] != GX_NONE) {
        mVertexSource += std::format("//layout (location = {}) in uint {};\n", currentLoc, VertexAttributeStrings[GX_VA_PNMTXIDX]);
        currentLoc++;
    }
    
    // Handle texmtxidx... attrs
    int numTexMtxEnabled = 0;
    for(int attr=GX_VA_TEX0MTXIDX; attr<=GX_VA_TEX7MTXIDX; attr++) {
        if(mDescriptors[attr] != GX_NONE) {
            numTexMtxEnabled++;
        }
    }

    if(numTexMtxEnabled > 0) {
        mVertexSource += std::format("//layout(location = {}) in uvec4 texMtx0;\n", currentLoc);
        currentLoc++;
        if(numTexMtxEnabled > 4) {
            mVertexSource += std::format("//layout(location = {}) in uvec4 texMtx1;\n", currentLoc);
            currentLoc++;
        }
    }

    for(int attr=GX_VA_POS; attr < GX_VA_MAX_ATTR; attr++) {
        if(mDescriptors[attr] != GX_NONE) {
            mVertexSource += std::format("//layout (location = {}) in {} {};\n",
                currentLoc, 
                GetTypeName(mDescriptors[attr], mFormat.mAttributes[attr].mDataType, mFormat.mAttributes[attr].mComponents), 
                VertexAttributeStrings[attr]);
            mAttrLocations[attr] = currentLoc;
            currentLoc++;
        } else {
            mAttrLocations[attr] = -1;
        }
    }

    // No vertex attributes enabled. we need to make something that compiles and links
    // but not display anything
    if(currentLoc == 0) {

    }

    // Add outputs
    mVertexSource += "//smooth out vec3 rasc;\n\
                      //smooth out float rasa;\n\
                      //smooth out vec2 gxTexCoords[8 /* GX_MAX_TEXCOORD */];\n";

    printf("Compiling shader: \n%s\n\n=====\n", mVertexSource.c_str());

    mVertexSource += std::string(SIM_GXVertexShader);
}

// Fragment shader will be constant for now
// it can easily be changed to templated later if necessary
void Shader::GenerateFragmentSource() {
    mFragmentSource = std::string(SIM_GXFragmentShader);
}

void Shader::Compile() {
    mVertexShader = glCreateShader(GL_VERTEX_SHADER);
    CompileShader(mVertexShader, mVertexSource.c_str());

    mFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    CompileShader(mFragmentShader, mFragmentSource.c_str());

    mProgram = glCreateProgram();
    LinkShader(mProgram, mVertexShader, mFragmentShader);
}

void Shader::SetupUniformLocations() {
    mProjectionLocation = glGetUniformLocation(static_cast<GLuint>(mProgram), "u_projection");
    mNormalMtxLocation = glGetUniformLocation(static_cast<GLuint>(mProgram), "u_normalMtx");
    mNumTexGenLocation = glGetUniformLocation(static_cast<GLuint>(mProgram), "u_numTexGens");
    for(int i=0; i < GX_MAX_TEXCOORD; i++) {
        mTexGenMatrixLocation[i] = glGetUniformLocation(static_cast<GLuint>(mProgram), std::format("u_texGens[{}].mMatrixId", i).c_str());
        mTexGenTypeLocation[i] = glGetUniformLocation(static_cast<GLuint>(mProgram), std::format("u_texGens[{}].mType", i).c_str());
    }


    mTevTexMapLocation = glGetUniformLocation(static_cast<GLuint>(mProgram), "tevTexMaps");
    mTevStageConfigsBinding = 0;
    glUniformBlockBinding(mProgram, mTevStageConfigsBlock, mTevStageConfigsBinding);
    mLightConfigBlock = glGetUniformBlockIndex(mProgram, "lightConfigBlock");
    mLightConfigBlockBinding = 1;
    glUniformBlockBinding(mProgram, mLightConfigBlock, mLightConfigBlockBinding);
    mMatrixMemoryBlock = glGetUniformBlockIndex(mProgram, "matrixMemoryBlock");
    mMatrixMemoryBlockBinding = 2;
    glUniformBlockBinding(mProgram, mMatrixMemoryBlock, mMatrixMemoryBlockBinding);
    mInitialTevColorsLocation =
        glGetUniformLocation(static_cast<GLuint>(mProgram), "initialTevColors");
    mNumTevStagesLocation =
        glGetUniformLocation(static_cast<GLuint>(mProgram), "numTevStages");
    mNumChansLocation = glGetUniformLocation(static_cast<GLuint>(mProgram), "u_numChans");
    mMtxIdxALocation = glGetUniformLocation(static_cast<GLuint>(mProgram), "mtxIdxA");
    mPnMtxIdxEnabledLocation = glGetUniformLocation(static_cast<GLuint>(mProgram), "pnMtxIdxEnabled");
}



ShaderCache::ShaderCache() {

}

std::shared_ptr<Shader> ShaderCache::GetShader(const SIM::GX::VertexFormat& format, const std::array<GXAttrType, GX_VA_MAX_ATTR>& descriptors) {
    u64 hash = HashAttributes(format, descriptors);

    if(mShaderMap.count(hash)) {
        return mShaderMap[hash];
    }

    return nullptr;
}

void ShaderCache::AddShader(const SIM::GX::VertexFormat& format, const std::array<GXAttrType, GX_VA_MAX_ATTR>& descriptors, std::shared_ptr<Shader> shaderPtr) {
    u64 hash = HashAttributes(format, descriptors);

    if(mShaderMap.count(hash)) {
        // tried to add a duplicate shader. this probably shouldnt happen
        return;
    }

    mShaderMap[hash] = shaderPtr;
}

u64 ShaderCache::HashAttributes(const SIM::GX::VertexFormat& format, const std::array<GXAttrType, GX_VA_MAX_ATTR>& descriptors) {
    u8 * vertexFormatBytes = (u8*)(&format);
    u8 * descriptorBytes = (u8*)(descriptors.data());

    u64 crcLo = SIM_crc32buf(vertexFormatBytes, sizeof(VertexFormat));
    u64 crcHi = SIM_crc32buf(descriptorBytes, sizeof(GXAttrType) * GX_VA_MAX_ATTR);

    return crcLo | (crcHi << 32);
}

}