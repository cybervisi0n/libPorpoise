#include <simulator/sim_gx_GlRenderer.hpp>

#include <cstddef>
#include <cstring>
#include <format>
#include <vector>
#include <string.h>

#include <simulator/glad/glad.h>
#include <simulator/sim_gx_Geometry.hpp>
#include <simulator/sim_gx_State.hpp>
#include <simulator/sim_gx_TextureManager.hpp>
#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

namespace {

static constexpr auto InitialRenderVertsCapacity = 1024;

GLenum ToGlPrimitive(GXPrimitive primitive) {
    switch (primitive) {
        case GX_POINTS:
            return GL_POINTS;
        case GX_LINES:
            return GL_LINES;
        case GX_LINESTRIP:
            return GL_LINE_STRIP;
        case GX_TRIANGLESTRIP:
            return GL_TRIANGLE_STRIP;
        case GX_TRIANGLEFAN:
            return GL_TRIANGLE_FAN;
        case GX_TRIANGLES:
        case GX_QUADS:
        case GX_QUADSTRIP:
        default:
            return GL_TRIANGLES;
    }
}

}

namespace SIM::GX {

void GlRenderer::Initialize() {
    if (mVertexArray != 0) {
        return;
    }

    mRenderVerts = new RenderVertex[InitialRenderVertsCapacity];
    mRenderVertsCount = 0;
    mRenderVertsCapacity = InitialRenderVertsCapacity;

    glGenVertexArrays(1, &mVertexArray);
    glGenBuffers(1, &mVertexBuffer);
    glBindVertexArray(mVertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);

    //location = 0 in vec3 position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(RenderVertex),
        reinterpret_cast<void*>(offsetof(RenderVertex, position)));
    //location = 1 in vec3 normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(RenderVertex),
        reinterpret_cast<void*>(offsetof(RenderVertex, normal)));
    //location = 2 in vec4 vertex_color
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        2,
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof(RenderVertex),
        reinterpret_cast<void*>(offsetof(RenderVertex, color0)));
    //location = 3 in vec2 texCoords
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(
        3,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(RenderVertex),
        reinterpret_cast<void*>(offsetof(RenderVertex, texCoords)));
    //location = 4 in uint posNormalMtxIdx
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(
        4,
        1,
        GL_UNSIGNED_INT,
        GL_FALSE,
        sizeof(RenderVertex),
        reinterpret_cast<void*>(offsetof(RenderVertex, posNormalMtxIdx)));
    //location = 5 in uvec4 texMtxIdx0
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(
        5,
        4,
        GL_UNSIGNED_INT,
        GL_FALSE,
        sizeof(RenderVertex),
        reinterpret_cast<void*>(offsetof(RenderVertex, texMtxIdx)));
    //location = 6 in uvec4 texMtxIdx1
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(
        6,
        4,
        GL_UNSIGNED_INT,
        GL_FALSE,
        sizeof(RenderVertex),
        reinterpret_cast<void*>(offsetof(RenderVertex, texMtxIdx) + (4 * sizeof(u32))));

    // Element buffer
    glGenBuffers(1, &mElementBuffer);
    
    // Allocate tev stage uniform buffer
    glGenBuffers(1, &mTevStageUniformBuffer);
    glBindBuffer(GL_UNIFORM_BUFFER, mTevStageUniformBuffer);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(TevStageConfig) * GX_MAX_TEVSTAGE, NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    
    // Allocate lights uniform buffer
    glGenBuffers(1, &mLightsUniformBuffer);
    glBindBuffer(GL_UNIFORM_BUFFER, mLightsUniformBuffer);
    glBufferData(GL_UNIFORM_BUFFER, (sizeof(Light) * 8) + (sizeof(ColorChannel) * 4), NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Allocate matrix memory uniform buffer
    glGenBuffers(1, &mMatrixMemoryUniformBuffer);
    glBindBuffer(GL_UNIFORM_BUFFER, mMatrixMemoryUniformBuffer);
    glBufferData(GL_UNIFORM_BUFFER, (sizeof(float) * 240) + (sizeof(float) * 120), NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    GLint shaderProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &shaderProgram);

    mProjectionLocation = glGetUniformLocation(static_cast<GLuint>(shaderProgram), "u_projection");
    mNormalMtxLocation = glGetUniformLocation(static_cast<GLuint>(shaderProgram), "u_normalMtx");
    mNumTexGenLocation = glGetUniformLocation(static_cast<GLuint>(shaderProgram), "u_numTexGens");
    for(int i=0; i < GX_MAX_TEXCOORD; i++) {
        mTexGenMatrixLocation[i] = glGetUniformLocation(static_cast<GLuint>(shaderProgram), std::format("u_texGens[{}].mMatrixId", i).c_str());
        mTexGenTypeLocation[i] = glGetUniformLocation(static_cast<GLuint>(shaderProgram), std::format("u_texGens[{}].mType", i).c_str());
    }


    mTevTexMapLocation = glGetUniformLocation(static_cast<GLuint>(shaderProgram), "tevTexMaps");
    mTevStageConfigsBinding = 0;
    glUniformBlockBinding(shaderProgram, mTevStageConfigsBlock, mTevStageConfigsBinding);
    mLightConfigBlock = glGetUniformBlockIndex(shaderProgram, "lightConfigBlock");
    mLightConfigBlockBinding = 1;
    glUniformBlockBinding(shaderProgram, mLightConfigBlock, mLightConfigBlockBinding);
    mMatrixMemoryBlock = glGetUniformBlockIndex(shaderProgram, "matrixMemoryBlock");
    mMatrixMemoryBlockBinding = 2;
    glUniformBlockBinding(shaderProgram, mMatrixMemoryBlock, mMatrixMemoryBlockBinding);
    mInitialTevColorsLocation =
        glGetUniformLocation(static_cast<GLuint>(shaderProgram), "initialTevColors");
    mNumTevStagesLocation =
        glGetUniformLocation(static_cast<GLuint>(shaderProgram), "numTevStages");
    mNumChansLocation = glGetUniformLocation(static_cast<GLuint>(shaderProgram), "u_numChans");
    mMtxIdxALocation = glGetUniformLocation(static_cast<GLuint>(shaderProgram), "mtxIdxA");
    mPnMtxIdxEnabledLocation = glGetUniformLocation(static_cast<GLuint>(shaderProgram), "pnMtxIdxEnabled");
}

void GlRenderer::Draw(const RenderVertex * vertices, size_t numVertices, GXPrimitive primitive) {
    #ifdef TRACY_ENABLE
    ZoneScoped;
    #endif
    if (numVertices == 0) {
        return;
    }

    Initialize();
    auto& gxState = GetGlobalState();

    mTotalDrawcalls++;


    #if 0
    // Dirty state logging
    {
        bool dirtyStates[10];

        dirtyStates[0] = gxState.GetIsTextureDirty();
        dirtyStates[1] = gxState.GetIsDepthDirty();
        dirtyStates[2] = gxState.GetIsProjectionMatrixDirty();
        dirtyStates[3] = gxState.GetIsTexGenDirty();
        dirtyStates[4] = gxState.GetTevDirty();
        dirtyStates[5] = gxState.GetIsTevTexMapDirty();
        dirtyStates[6] = gxState.GetIsLightsDirty();
        dirtyStates[7] = gxState.GetIsPosTextureMtxDirty();
        dirtyStates[8] = gxState.GetIsNormalMtxDirty();

        printf("GX: %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n", 
            numVertices,
            dirtyStates[0],
            dirtyStates[1],
            dirtyStates[2],
            dirtyStates[3],
            dirtyStates[4],
            dirtyStates[5],
            dirtyStates[6],
            dirtyStates[7],
            dirtyStates[8]);
    }
    #endif

    bool batchable = true;

    if(
        gxState.GetIsTextureDirty() ||
        gxState.GetIsDepthDirty() ||
        gxState.GetIsProjectionMatrixDirty() ||
        gxState.GetIsTexGenDirty() ||
        gxState.GetTevDirty() ||
        gxState.GetIsTevTexMapDirty() ||
        gxState.GetIsLightsDirty() ||
        gxState.GetIsPosTextureMtxDirty() ||
        gxState.GetIsNormalMtxDirty() ||
        gxState.GetIsNumChannelsDirty() ||
        gxState.GetIsNumTevStagesDirty() ||
        gxState.GetIsInitialTevColorsDirty() ||
        gxState.GetIsMatrixIndexDirty() ||
        ((u32)ToGlPrimitive(primitive) != (u32)mRenderVertsPrimitive) ||
        (mRenderVertsIndexed != IsIndexed(primitive))
    ) {
        // This Drawcall is not batchable. Flush mRenderVerts now
        batchable = false;
        FlushRenderVerts();
    }

    if(gxState.GetIsTextureDirty()) {
        TextureManager::GetInstance().ProcessTextures();
        gxState.SetTextureDirty(false);
    }

    if(gxState.GetIsDepthDirty()) {
        if(gxState.GetDepthCompareEnabled()) {
            glEnable(GL_DEPTH_TEST);
        } else {
            glDisable(GL_DEPTH_TEST);
        }

        //if(gxState.GetDepthUpdateEnabled()) {
        //    glDepthMask(GL_TRUE);
        //} else {
        //    glDepthMask(GL_FALSE);
        //}
        gxState.SetDepthDirty(false);
    }

    size_t numDrawVertices = numVertices;

    mRenderVertsIndexed = IsIndexed(primitive);
    if (primitive == GX_QUADS) {
        ExpandQuads(vertices, numVertices);
    } else if (primitive == GX_QUADSTRIP) {
        ExpandQuadStrip(vertices, numVertices);
    } else {
        ReserveRenderVerts(numVertices);
        memcpy(&mRenderVerts[mRenderVertsCount], vertices, sizeof(RenderVertex) * numVertices);
        mRenderVertsCount += numVertices;
    }

    if (numDrawVertices == 0 || batchable) {
        return;
    }

    mRenderVertsPrimitive = ToGlPrimitive(primitive);

    //GLint shaderProgram = 0;
    //glGetIntegerv(GL_CURRENT_PROGRAM, &shaderProgram);
    //if (shaderProgram == 0) {
    //    return;
    //}

    
    //glUseProgram(shaderProgram);

    if(gxState.GetIsProjectionMatrixDirty()) {
        glUniformMatrix4fv(
            mProjectionLocation,
            1,
            GL_TRUE,
            gxState.GetProjectionMatrix().data());
        gxState.SetProjectionMatrixDirty(false);
    }

    if(gxState.GetIsTexGenDirty()) {
        glUniform1ui(mNumTexGenLocation, gxState.GetNumTexGens());
        for(int i=0; i < GX_MAX_TEXCOORD; i++) {
            glUniform1ui(mTexGenMatrixLocation[i], gxState.GetTexGenArray()[i].mMatrixId);
            glUniform1ui(mTexGenTypeLocation[i], gxState.GetTexGenArray()[i].mType);
        }
        gxState.SetTexGenDirty(false);
    }


    if(gxState.GetIsTevTexMapDirty()) {
        glUniform1iv(mTevTexMapLocation, GX_MAX_TEVSTAGE, (const GLint*)gxState.GetTevTexMapArray());
        gxState.SetTevTexMapDirty(false);
    }
    

    if(gxState.GetTevDirty()) {
        glBindBuffer(GL_UNIFORM_BUFFER, mTevStageUniformBuffer);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(TevStageConfig) * GX_MAX_TEVSTAGE, gxState.GetTevStageConfigArray());
        glBindBufferBase(GL_UNIFORM_BUFFER, mTevStageConfigsBinding, mTevStageUniformBuffer);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        gxState.SetTevDirty(false);
    }

    //upload matrix memory position + texture
    if(gxState.GetIsPosTextureMtxDirty() || gxState.GetIsNormalMtxDirty()) {
        glBindBuffer(GL_UNIFORM_BUFFER, mMatrixMemoryUniformBuffer);
        //const float * matrixMem = gxState.GetXfMemoryPointer();
        glBufferSubData(GL_UNIFORM_BUFFER, 0, 240 * sizeof(float), gxState.GetXfMemoryPointer());
        glBindBufferBase(GL_UNIFORM_BUFFER, mMatrixMemoryBlockBinding, mMatrixMemoryUniformBuffer);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        gxState.SetPosTextureMtxDirty(false);
    }

    if(gxState.GetIsNormalMtxDirty()) {
        // upload matrix memory normal mtx
        glBindBuffer(GL_UNIFORM_BUFFER, mMatrixMemoryUniformBuffer);
        const float * matrixMem = gxState.GetXfMemoryPointer() + 0x400;
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
        gxState.SetNormalMtxDirty(false);
    }

    // pass lights data
    if(gxState.GetIsLightsDirty()) {
        glBindBuffer(GL_UNIFORM_BUFFER, mLightsUniformBuffer);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Light) * 8, gxState.GetLightsArray());
        glBufferSubData(GL_UNIFORM_BUFFER, sizeof(Light) * 8, sizeof(ColorChannel) * 4, gxState.GetColorChannelArray());
        glBindBufferBase(GL_UNIFORM_BUFFER, mLightConfigBlockBinding, mLightsUniformBuffer);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        gxState.SetLightsDirty(false);
    }

    if(gxState.GetIsMatrixIndexDirty()) {
        glUniform1ui(mMtxIdxALocation, gxState.GetCurrentPositionMtxIdx());
        gxState.SetMatrixIndexDirty(false);
    }

    glUniform1ui(mPnMtxIdxEnabledLocation, gxState.GetVertexDescriptor(GX_VA_PNMTXIDX) != GX_NONE);

    if(gxState.GetIsNumChannelsDirty()) {
        glUniform1ui(mNumChansLocation, gxState.GetNumChannels());
        gxState.SetNumChannelsDirty(false);
    }

    if(gxState.GetIsInitialTevColorsDirty()) {
        glUniform4fv(mInitialTevColorsLocation, 4, gxState.GetInitialTevColorsArray());
        gxState.SetInitialTevColorsDirty(false);
    }

    if(gxState.GetIsNumTevStagesDirty()) {
        glUniform1ui(mNumTevStagesLocation, gxState.GetNumTevStages());
        gxState.SetNumTevStagesDirty(false);
    }
}

bool GlRenderer::IsIndexed(GXPrimitive prim) {
    switch(prim) {
        case GX_LINESTRIP:
        case GX_QUADS:
        case GX_QUADSTRIP:
            return true;
        case GX_LINES:
        case GX_POINTS:
        case GX_TRIANGLES:
        case GX_TRIANGLEFAN:
        case GX_TRIANGLESTRIP:
        default:
            return false;
    }
}

void GlRenderer::ExpandQuads(
    const SIM::GX::RenderVertex * vertices, size_t numVertices) {
    int numVertsOut = (numVertices / 4) * 6;
    ReserveRenderVerts(numVertices);
    SIM::GX::RenderVertex * renderVertsOut = &mRenderVerts[mRenderVertsCount];
    std::memcpy(renderVertsOut, vertices, sizeof(SIM::GX::RenderVertex) * numVertices);
    u32 oldVertexCount = mRenderVertsCount;
    mRenderVertsCount += numVertices;

    ReserveRenderIndices(numVertsOut);
    u32 * indices = &mRenderIndices[mRenderIndicesCount];
    size_t trianglesIdx = 0;
    for (size_t i = 0; i + 3 < numVertices; i += 4) {
        indices[trianglesIdx++] = oldVertexCount + i;
        indices[trianglesIdx++] = oldVertexCount + i + 1;
        indices[trianglesIdx++] = oldVertexCount + i + 2;
        indices[trianglesIdx++] = oldVertexCount + i;
        indices[trianglesIdx++] = oldVertexCount + i + 2;
        indices[trianglesIdx++] = oldVertexCount + i + 3;
    }

    mRenderIndicesCount += numVertsOut;
}

void GlRenderer::ExpandQuadStrip(
    const SIM::GX::RenderVertex * vertices, size_t numVertices) {
    ReserveRenderVerts(numVertices);
    SIM::GX::RenderVertex * renderVertsOut = &mRenderVerts[mRenderVertsCount];
    std::memcpy(renderVertsOut, vertices, sizeof(SIM::GX::RenderVertex) * numVertices);
    u32 oldVertexCount = mRenderVertsCount;
    mRenderVertsCount += numVertices;

    if (numVertices < 4) {
        ReserveRenderIndices(numVertices);
        for(size_t i = 0; i < numVertices; i++) {
            mRenderIndices[oldVertexCount + i] = oldVertexCount + i;
        }
        mRenderIndicesCount += numVertices;
        return;
    }

    int numVertsOut = ((numVertices - 2) / 2) * 6;
    ReserveRenderIndices(numVertsOut);

    u32 * indices = &mRenderIndices[mRenderIndicesCount];
    size_t trianglesIdx = 0;
    for(size_t i=0; i + 3 < numVertices; i+= 2) {
        indices[trianglesIdx++] = oldVertexCount + i;
        indices[trianglesIdx++] = oldVertexCount + i + 1;
        indices[trianglesIdx++] = oldVertexCount + i + 2;
        indices[trianglesIdx++] = oldVertexCount + i;
        indices[trianglesIdx++] = oldVertexCount + i + 2;
        indices[trianglesIdx++] = oldVertexCount + i + 3;

    }
    mRenderIndicesCount += numVertsOut;
}

void GlRenderer::ReserveRenderVerts(int numAdditionalVerts) {
    if(mRenderVertsCount + numAdditionalVerts < mRenderVertsCapacity) {
        return;
    }

    RenderVertex * renderVertsNew = new RenderVertex[mRenderVertsCount + numAdditionalVerts];
    memcpy(renderVertsNew, mRenderVerts, sizeof(RenderVertex) * mRenderVertsCount);
    delete mRenderVerts;
    mRenderVerts = renderVertsNew;
    mRenderVertsCapacity = mRenderVertsCount + numAdditionalVerts;
}

void GlRenderer::ReserveRenderIndices(int numAdditionalIndices) {
    if(mRenderIndicesCount + numAdditionalIndices < mRenderIndicesCapacity) {
        return;
    }

    u32 * renderIndicesNew = new u32[mRenderIndicesCount + numAdditionalIndices];
    memcpy(renderIndicesNew, mRenderVerts, sizeof(u32) * mRenderIndicesCount);
    delete mRenderIndices;
    mRenderIndices = renderIndicesNew;
    mRenderIndicesCapacity = mRenderIndicesCount + numAdditionalIndices;
}

void GlRenderer::FlushRenderVerts() {
    if(mRenderVertsCount == 0) {
        return;
    }
    #ifdef TRACY_ENABLE
    ZoneScoped;
    #endif
    glBindVertexArray(mVertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
    if(mVboCapacity < mRenderVertsCount) {
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(mRenderVertsCount * sizeof(RenderVertex)),
            mRenderVerts,
            GL_STREAM_DRAW);
        mVboCapacity = mRenderVertsCount;
    } else {
        glBufferSubData(
            GL_ARRAY_BUFFER,
            0,
            static_cast<GLsizeiptr>(mRenderVertsCount * sizeof(RenderVertex)),
            mRenderVerts
        );
    }

    if(mRenderVertsIndexed) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mElementBuffer);
        if(mEboCapacity < mRenderIndicesCount) {
            glBufferData(
                GL_ELEMENT_ARRAY_BUFFER,
                static_cast<GLsizeiptr>(mRenderIndicesCount * sizeof(u32)),
                mRenderIndices,
                GL_STREAM_DRAW);
        } else {
            glBufferSubData(
                GL_ELEMENT_ARRAY_BUFFER,
                0,
                static_cast<GLsizeiptr>(mRenderIndicesCount * sizeof(u32)),
                mRenderIndices
            );
        }

        glDrawElements(mRenderVertsPrimitive, mRenderIndicesCount, GL_UNSIGNED_INT, nullptr);
    } else {
        glDrawArrays(
            mRenderVertsPrimitive,
            0,
            static_cast<GLsizei>(mRenderVertsCount));
    }

    
    mRenderVertsCount = 0;
    mRenderIndicesCount = 0;
}

GlRenderer& GetGlRenderer() {
    static GlRenderer renderer;
    return renderer;
}

}
