#include "simulator/sim_gx_Geometry.hpp"

#include <cmath>
#include <cstring>

#include <dolphin.h>

#include "dolphin/gx/GXEnum.h"
#include "simulator/sim_crc32.h"
#include "simulator/sim_gx_GlRenderer.hpp"
#include "simulator/sim_gx_State.hpp"
#include "simulator/byteswap.h"
#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

#include "simulator/sim_gx_VertexDecoderGeneric.hpp"
#include "simulator/sim_gx_VertexDecoderOnetri.hpp"

namespace {

}

namespace SIM::GX {

GeometryProcessor::GeometryProcessor() {}

void GeometryProcessor::ProcessByteStream(std::vector<u8>& byteStream, std::endian endian) {
    #ifdef TRACY_ENABLE
    ZoneScoped;
    #endif
    
    auto& gxState = GetGlobalState();
    //mRenderVerts.clear();

    if(gxState.GetIsVertexAttributesDirty()) {
        mNumBytesPerVertex = gxState.GetNumBytesPerVertex();

        //determine if we can use a specialized vertex decoder
        const auto& vtxFormat = gxState.GetCurrentVertexFormat();
        const u8* vtxFormatBytes = (const u8*)(&vtxFormat);
        u32 vertexCRC = SIM_crc32buf(vtxFormatBytes, sizeof(VertexFormat));

        const auto& vtxDescriptors = gxState.GetVertexDescriptorArray();
        const u8 * vertexDescriptorBytes = (const u8*)(vtxDescriptors.data());
        vertexCRC = SIM_updateCRC32buf(vertexCRC, vertexDescriptorBytes, sizeof(GXAttrType) * GX_VA_MAX_ATTR);

        // onetri example 3229902983

        if(vertexCRC == 3229902983) {
            printf("Loading onetri example vertex decoder\n");
            mVertexDecoder = std::make_shared<VertexDecoderOnetri>();
            //blah
        } else {
            printf("Loading generic vertex interpreter\n");
            mVertexDecoder = std::make_shared<VertexDecoderGeneric>(vtxFormat, vtxDescriptors.data(), mNumBytesPerVertex);
        }
    }

    if (mNumBytesPerVertex == 0 || byteStream.empty() ||
        byteStream.size() % mNumBytesPerVertex != 0) {
        return;
    }

    const size_t numVertices = byteStream.size() / mNumBytesPerVertex;
    if(numVertices > mRenderVertsSize) {
        // Reallocate mRenderVerts
        if(mRenderVerts) {
            delete mRenderVerts;
        }
        mRenderVerts = new RenderVertex[numVertices];
        mRenderVertsSize = numVertices;
    }

    mVertexDecoder->DecodeVerts(byteStream.data(), mRenderVerts, numVertices, endian);

    GetGlRenderer().Draw(mRenderVerts, numVertices, gxState.GetCurrentPrimitive());
}
}
