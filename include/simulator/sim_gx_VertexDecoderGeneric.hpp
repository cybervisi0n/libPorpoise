#ifndef LIBPORPOISE_SIM_GX_VERTEXDECODERGENERIC_HPP
#define LIBPORPOISE_SIM_GX_VERTEXDECODERGENERIC_HPP

#include "simulator/sim_gx_IVertexDecoder.hpp"
#include "simulator/sim_gx_State.hpp"

#include <array>

namespace SIM::GX {

class VertexDecoderGeneric : public IVertexDecoder {
    public:
        VertexDecoderGeneric(const VertexFormat& format, const GXAttrType * descriptors, size_t bytesPerVertex);
        virtual void DecodeVerts(u8 * byteStream, RenderVertex * vertsOut, size_t numVerts, std::endian endian);
        virtual ~VertexDecoderGeneric() {};
    
    private:

        struct VertexInfo {
            GXAttrType mDescriptor;
            size_t mComponentCount;
            size_t mComponentSize;
            VertexArray mVertexArray;
        };
        VertexInfo mVtxInfo[GX_VA_MAX_ATTR] = {};
        size_t mBytesPerVertex = 0;
        VertexFormat mFormat;
        std::array<GXAttrType, GX_VA_MAX_ATTR> mDescriptors;
};

}

#endif
