#ifndef LIBPORPOISE_SIM_GX_VERTEXDECODERONETRI_HPP
#define LIBPORPOISE_SIM_GX_VERTEXDECODERONETRI_HPP

#include "simulator/sim_gx_IVertexDecoder.hpp"

namespace SIM::GX {

class VertexDecoderOnetri : public IVertexDecoder {
    public:
        VertexDecoderOnetri();
        virtual void DecodeVerts(u8 * byteStream, RenderVertex * vertsOut, size_t numVerts, std::endian endian);
        virtual ~VertexDecoderOnetri() {};
    
    private:
        struct BinaryVertex {
            u16 position;
            u16 color0;
        };
        static constexpr int BytesPerVertex = 4;
};

}

#endif
