#include "simulator/sim_gx_VertexDecoderOnetri.hpp"

#include "dolphin/GX/GXEnum.h"
#include "dolphin/OS.h"
#include "simulator/byteswap.h"
#include "simulator/sim_gx_Geometry.hpp"
#include "simulator/sim_gx_State.hpp"

#include <bit>
#include <cstring>

namespace SIM::GX {

VertexDecoderOnetri::VertexDecoderOnetri(){
    
}

void VertexDecoderOnetri::DecodeVerts(u8 * byteStream, RenderVertex * vertsOut, size_t numVerts, std::endian endian) {
    //GX_VA_POS: INDEX16
    //GX_VA_CLR0: INDEX16

    // pos: xyz (s16) frac 0
    // color0: rgba8 (u8)
    const BinaryVertex * vertsIn = (const BinaryVertex * )(byteStream);
    auto& gxState = GetGlobalState();

    // get any vertex arrays we need
    const auto& posArray = gxState.GetVertexArray(GX_VA_POS);
    const auto& clr0Array = gxState.GetVertexArray(GX_VA_CLR0);

    u8 * cursor;
    s16 * data;
    for(size_t i=0; i < numVerts; i++) {
        const auto& vertIn = vertsIn[i];
        auto& vertOut = vertsOut[i];

        // position
        cursor = (u8*)posArray.mArrayPtr + (posArray.mStride * vertIn.position);
        data = (s16*)cursor;
        vertOut.position.x = (float)(data[0]);
        vertOut.position.y = (float)(data[1]);
        vertOut.position.z = (float)(data[2]);

        // color0
        cursor = (u8*)clr0Array.mArrayPtr + (clr0Array.mStride * vertIn.color0);
        vertOut.color0.r = (float)(cursor[0]) / 255.0f;
        vertOut.color0.g = (float)(cursor[1]) / 255.0f;
        vertOut.color0.b = (float)(cursor[2]) / 255.0f;
        vertOut.color0.a = (float)(cursor[3]) / 255.0f;
    }
}

}