#include <simulator/sim_gx_GlRenderer.hpp>

#include <cstddef>
#include <cstring>
#include <format>
#include <vector>
#include <string.h>

#include <simulator/glad/glad.h>
#include <simulator/sim_gx_Geometry.hpp>
#include <simulator/sim_gx_Shader.hpp>
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
        case GX_TRIANGLEFAN:
            return GL_TRIANGLE_FAN;
        case GX_TRIANGLES:
        case GX_TRIANGLESTRIP:
        case GX_QUADS:
        case GX_QUADSTRIP:
        default:
            return GL_TRIANGLES;
    }
}

static int GetGlType(GXAttr attr, GXCompType compType, GXAttrType descriptor) {
    if(descriptor == GX_INDEX8) {
        return GL_UNSIGNED_BYTE;
    } else if(descriptor == GX_INDEX16) {
        return GL_UNSIGNED_SHORT;
    }

    switch(compType) {
        case GX_U8:
            return GL_UNSIGNED_BYTE;
        case GX_U16:
            return GL_UNSIGNED_SHORT;
        default:
            break;
    }
    
    return GL_FLOAT;
}

static int GetNumComponents(GXAttr attr) {
    auto& gxState = SIM::GX::GetGlobalState();
    auto& attrStruct = gxState.GetCurrentVertexFormat().mAttributes[attr];
    switch(attr) {
        case GX_VA_POS:
            return gxState.GetNumPositionComponents(attrStruct.mComponents);
        case GX_VA_NRM:
            return gxState.GetNumNormalComponents(attrStruct.mComponents);
        case GX_VA_CLR0:
        case GX_VA_CLR1:
            return gxState.GetNumColorComponents(attrStruct.mComponents);
        case GX_VA_TEX0:
        case GX_VA_TEX1:
        case GX_VA_TEX2:
        case GX_VA_TEX3:
        case GX_VA_TEX4:
        case GX_VA_TEX5:
        case GX_VA_TEX6:
        case GX_VA_TEX7:
            return gxState.GetNumTexCoordComponents(attrStruct.mComponents);
        default:
            return 1;
    }
}

}

namespace SIM::GX {

void GlRenderer::Initialize() {
    if (mVertexArray != 0) {
        return;
    }

    mNumBytesPerVertex = GetGlobalState().GetNumBytesPerVertex();
    mRenderVerts = new u8[InitialRenderVertsCapacity * mNumBytesPerVertex];
    mRenderVertsCount = 0;
    mRenderVertsCapacity = InitialRenderVertsCapacity;
    mRenderIndices = new u32[InitialRenderVertsCapacity];
    mRenderIndicesCount = 0;
    mRenderIndicesCapacity = InitialRenderVertsCapacity;

    glGenVertexArrays(1, &mVertexArray);
    glGenBuffers(1, &mVertexBuffer);
    glBindVertexArray(mVertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);

    /*
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
    */
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

    // Create default shader
    auto& gxState = GetGlobalState();
    const auto& curFormat = gxState.GetCurrentVertexFormat();
    const auto& curDescriptors = gxState.GetVertexDescriptorArray();
    mCurrentShader = std::make_shared<Shader>(
        curFormat, 
        curDescriptors,
        mTevStageUniformBuffer,
        mLightsUniformBuffer,
        mMatrixMemoryUniformBuffer);
    mCurrentShader->Activate();

    mShaderCache = new ShaderCache();
}

void GlRenderer::Draw(const u8 * vertices, size_t numVertices, GXPrimitive primitive) {
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
        gxState.GetIsVertexAttributesDirty() ||
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

    if(gxState.GetIsVertexAttributesDirty()) {
        // do the shader cache stuff and load new shader
        const auto& curFormat = gxState.GetCurrentVertexFormat();
        const auto& curDescriptors = gxState.GetVertexDescriptorArray();
        mNumBytesPerVertex = gxState.GetNumBytesPerVertex();
        mCurrentShader = mShaderCache->GetShader(curFormat, curDescriptors);
        if(mCurrentShader == nullptr) {
            // Create the shader
            mCurrentShader = std::make_shared<Shader>(
                curFormat, 
                curDescriptors,
                mTevStageUniformBuffer,
                mLightsUniformBuffer,
                mMatrixMemoryUniformBuffer);
            mShaderCache->AddShader(curFormat, curDescriptors, mCurrentShader);
        }

        mCurrentShader->Activate();

        glBindVertexArray(mVertexArray);
        glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);

        // Disable all the attributes first
        for(int i=0; i < GL_MAX_VERTEX_ATTRIBS; i++) {
            glDisableVertexAttribArray(i);
        }

        // Set up the GL vertex attributes
        auto pnMtxAttrIdx = mCurrentShader->GetGlVertexAttrIdx(GX_VA_PNMTXIDX);
        if(pnMtxAttrIdx.has_value()) {
            glEnableVertexAttribArray(pnMtxAttrIdx.value());
            glVertexAttribPointer(
                pnMtxAttrIdx.value(),
                1, // num components
                GetGlType(GX_VA_PNMTXIDX, gxState.GetCurrentVertexFormat().mAttributes[GX_VA_PNMTXIDX].mDataType, gxState.GetVertexDescriptor(GX_VA_PNMTXIDX)), // get the correct type
                GL_FALSE,
                mNumBytesPerVertex,
                (void*)gxState.GetVertexAttrOffset(GX_VA_PNMTXIDX));            
        }

        ProcessIndexableAttribute(GX_VA_POS);
        ProcessIndexableAttribute(GX_VA_NRM);
        ProcessIndexableAttribute(GX_VA_CLR0);
        ProcessIndexableAttribute(GX_VA_CLR1);
        ProcessIndexableAttribute(GX_VA_TEX0);
        ProcessIndexableAttribute(GX_VA_TEX1);
        ProcessIndexableAttribute(GX_VA_TEX2);
        ProcessIndexableAttribute(GX_VA_TEX3);
        ProcessIndexableAttribute(GX_VA_TEX4);
        ProcessIndexableAttribute(GX_VA_TEX5);
        ProcessIndexableAttribute(GX_VA_TEX6);
        ProcessIndexableAttribute(GX_VA_TEX7);

        // notes: possibly consider storing the VAO with the shader object so these calculations only need done once (butt hat would mean Shader class does non-shader things)

        // do this for the remaining attributes

        // then refactor Draw() to take in the raw byte stream

        //int numTexMtxIdx = 0;
        //for(int i= GX_VA_TEX0MTXIDX; i<= GX_VA_TEX7MTXIDX; i++) {
        //    if(gxState.GetVertexDescriptor(static_cast<GXAttr>(i)) != GX_NONE) {
        //        numTexMtxIdx++;
        //    }
        //}

        // All uniforms will need to be set again with the new shader program
        // Mark all uniforms dirty
        gxState.SetTextureDirty(true);
        gxState.SetDepthDirty(true);
        gxState.SetProjectionMatrixDirty(true);
        gxState.SetTexGenDirty(true);
        gxState.SetTevDirty(true);
        gxState.SetTevTexMapDirty(true);
        gxState.SetLightsDirty(true);
        gxState.SetPosTextureMtxDirty(true);
        gxState.SetNormalMtxDirty(true);
        gxState.SetNumChannelsDirty(true);
        gxState.SetNumTevStagesDirty(true);
        gxState.SetInitialTevColorsDirty(true);
        gxState.SetMatrixIndexDirty(true);

        // When an application changes the vertex attributes this can be a very expensive operation

        gxState.SetVertexAttributesDirty(false);
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
    } else if (primitive == GX_TRIANGLESTRIP) {
        ExpandTriangleStrip(vertices, numVertices);  
    } else {
        ReserveRenderVerts(numVertices);
        memcpy(&mRenderVerts[mRenderVertsCount * mNumBytesPerVertex], vertices, mNumBytesPerVertex * numVertices);
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
        mCurrentShader->SetProjectionMatrix(gxState.GetProjectionMatrix().data());
        gxState.SetProjectionMatrixDirty(false);
    }

    if(gxState.GetIsTexGenDirty()) {
        mCurrentShader->SetNumTexGens(gxState.GetNumTexGens());
        for(int i=0; i < GX_MAX_TEXCOORD; i++) {
            mCurrentShader->SetTexGen(gxState.GetTexGenArray()[i], i);
        }
        gxState.SetTexGenDirty(false);
    }


    if(gxState.GetIsTevTexMapDirty()) {
        mCurrentShader->SetTevTexMaps(gxState.GetTevTexMapArray());
        gxState.SetTevTexMapDirty(false);
    }
    

    if(gxState.GetTevDirty()) {
        mCurrentShader->SetTevStageConfigs(gxState.GetTevStageConfigArray());
        gxState.SetTevDirty(false);
    }

    //upload matrix memory position + texture
    if(gxState.GetIsPosTextureMtxDirty() || gxState.GetIsNormalMtxDirty()) {
        mCurrentShader->SetPosTextureMatrixMem(gxState.GetXfMemoryPointer());
        gxState.SetPosTextureMtxDirty(false);
    }

    if(gxState.GetIsNormalMtxDirty()) {
        // upload matrix memory normal mtx
        mCurrentShader->SetNormalMatrixMem(gxState.GetXfMemoryPointer());
        gxState.SetNormalMtxDirty(false);
    }

    // pass lights data
    if(gxState.GetIsLightsDirty()) {
        mCurrentShader->SetLights(gxState.GetLightsArray(), gxState.GetColorChannelArray());
        gxState.SetLightsDirty(false);
    }

    if(gxState.GetIsMatrixIndexDirty()) {
        mCurrentShader->SetMatrixIndex(gxState.GetCurrentPositionMtxIdx());
        gxState.SetMatrixIndexDirty(false);
    }

    //glUniform1ui(mPnMtxIdxEnabledLocation, gxState.GetVertexDescriptor(GX_VA_PNMTXIDX) != GX_NONE);

    if(gxState.GetIsNumChannelsDirty()) {
        mCurrentShader->SetNumChannels(gxState.GetNumChannels());
        gxState.SetNumChannelsDirty(false);
    }

    if(gxState.GetIsInitialTevColorsDirty()) {
        mCurrentShader->SetInitialTevColors(gxState.GetInitialTevColorsArray());
        gxState.SetInitialTevColorsDirty(false);
    }

    if(gxState.GetIsNumTevStagesDirty()) {
        mCurrentShader->SetNumTevStages(gxState.GetNumTevStages());
        gxState.SetNumTevStagesDirty(false);
    }
}

bool GlRenderer::IsIndexed(GXPrimitive prim) {
    switch(prim) {
        case GX_LINESTRIP:
        case GX_QUADS:
        case GX_QUADSTRIP:
        case GX_TRIANGLESTRIP:
            return true;
        case GX_LINES:
        case GX_POINTS:
        case GX_TRIANGLES:
        case GX_TRIANGLEFAN:
        default:
            return false;
    }
}

void GlRenderer::ExpandQuads(
    const u8 * vertices, size_t numVertices) {
    int numVertsOut = (numVertices / 4) * 6;
    ReserveRenderVerts(numVertices);
    u8 * renderVertsOut = &mRenderVerts[mRenderVertsCount * mNumBytesPerVertex ];
    std::memcpy(renderVertsOut, vertices, mNumBytesPerVertex * numVertices);
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
    const u8 * vertices, size_t numVertices) {
    ReserveRenderVerts(numVertices);
    u8 * renderVertsOut = &mRenderVerts[mRenderVertsCount * mNumBytesPerVertex ];
    std::memcpy(renderVertsOut, vertices, mNumBytesPerVertex * numVertices);
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

void GlRenderer::ExpandTriangleStrip(
    const u8 * vertices, size_t numVertices) {
    ReserveRenderVerts(numVertices);
    u8 * renderVertsOut = &mRenderVerts[mRenderVertsCount * mNumBytesPerVertex];
    std::memcpy(renderVertsOut, vertices, mNumBytesPerVertex  * numVertices);
    u32 oldVertexCount = mRenderVertsCount;
    mRenderVertsCount += numVertices;

    // A strip needs at least 3 vertices to form a triangle
    if (numVertices < 3) {
        return;
    }

    int numTriangles = numVertices - 2;
    int numIndicesOut = numTriangles * 3;
    ReserveRenderIndices(numIndicesOut);

    u32 * indices = &mRenderIndices[mRenderIndicesCount];
    size_t trianglesIdx = 0;
    for (size_t i = 0; i + 2 < numVertices; i++) {
        // Every other triangle has its first two vertices swapped so that
        // the winding order stays consistent across the whole strip
        if ((i & 1) == 0) {
            indices[trianglesIdx++] = oldVertexCount + i;
            indices[trianglesIdx++] = oldVertexCount + i + 1;
        } else {
            indices[trianglesIdx++] = oldVertexCount + i + 1;
            indices[trianglesIdx++] = oldVertexCount + i;
        }
        indices[trianglesIdx++] = oldVertexCount + i + 2;
    }

    mRenderIndicesCount += numIndicesOut;
}

void GlRenderer::ReserveRenderVerts(int numAdditionalVerts) {
    if(mRenderVertsCount + numAdditionalVerts < mRenderVertsCapacity) {
        return;
    }

    auto bytesPerVertex = mNumBytesPerVertex;

    u8 * renderVertsNew = new u8[(mRenderVertsCount + numAdditionalVerts) * bytesPerVertex];
    memcpy(renderVertsNew, mRenderVerts, bytesPerVertex * mRenderVertsCount);
    delete mRenderVerts;
    mRenderVerts = renderVertsNew;
    mRenderVertsCapacity = mRenderVertsCount + numAdditionalVerts;
}

void GlRenderer::ReserveRenderIndices(int numAdditionalIndices) {
    if(mRenderIndicesCount + numAdditionalIndices < mRenderIndicesCapacity) {
        return;
    }

    u32 * renderIndicesNew = new u32[mRenderIndicesCount + numAdditionalIndices];
    memcpy(renderIndicesNew, mRenderIndices, sizeof(u32) * mRenderIndicesCount);
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
            static_cast<GLsizeiptr>(mRenderVertsCount * mNumBytesPerVertex),
            mRenderVerts,
            GL_STREAM_DRAW);
        mVboCapacity = mRenderVertsCount;
    } else {
        glBufferSubData(
            GL_ARRAY_BUFFER,
            0,
            static_cast<GLsizeiptr>(mRenderVertsCount * mNumBytesPerVertex),
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

void GlRenderer::ProcessIndexableAttribute(GXAttr attr) {
    auto& gxState = GetGlobalState();
    auto attrIdx = mCurrentShader->GetGlVertexAttrIdx(attr);
    if(attrIdx.has_value()) {
        glEnableVertexAttribArray(attrIdx.value());
        auto descriptor = gxState.GetVertexDescriptor(attr);
        if(descriptor == GX_INDEX8 || descriptor == GX_INDEX16) {
            glVertexAttribIPointer(
                attrIdx.value(),
                1, // num components
                GetGlType(attr, gxState.GetCurrentVertexFormat().mAttributes[attr].mDataType, gxState.GetVertexDescriptor(attr)), // get the correct type
                mNumBytesPerVertex,
                (void*)gxState.GetVertexAttrOffset(attr)); 
        } else {
            glVertexAttribPointer(
                attrIdx.value(),
                GetNumComponents(attr), // num components
                GetGlType(attr, gxState.GetCurrentVertexFormat().mAttributes[attr].mDataType, gxState.GetVertexDescriptor(attr)), // get the correct type
                GL_FALSE,
                mNumBytesPerVertex,
                (void*)gxState.GetVertexAttrOffset(attr)); 
        }           
    }
}

GlRenderer& GetGlRenderer() {
    static GlRenderer renderer;
    return renderer;
}

}
