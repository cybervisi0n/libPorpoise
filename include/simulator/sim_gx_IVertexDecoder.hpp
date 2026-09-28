#ifndef LIBPORPOISE_SIM_GX_IVERTEXDECODER_HPP
#define LIBPORPOISE_SIM_GX_IVERTEXDECODER_HPP

#include <dolphin/types.h>
#include <bit>
#include <cstddef>

namespace SIM::GX {

struct RenderVertex;

class IVertexDecoder {
    public:
        virtual void DecodeVerts(u8 * byteStream, RenderVertex * vertsOut, size_t numVerts, std::endian endian) = 0;
        virtual ~IVertexDecoder() {};
};

}

#endif
