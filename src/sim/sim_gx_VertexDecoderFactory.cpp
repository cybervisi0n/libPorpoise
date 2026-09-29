#include <dolphin/types.h>
#include "simulator/sim_gx_Geometry.hpp"
#include "simulator/sim_gx_IVertexDecoder.hpp"
#include "simulator/sim_gx_State.hpp"
#include "simulator/byteswap.h"
#include <memory>
#include <cstring>

static inline bool IsByteswapRequired(std::endian endian) {
    return (std::endian::native != endian);
}

template <typename T>
static inline T ReadUnaligned(const u8* source, std::endian endian) {
    T value;
    std::memcpy(&value, source, sizeof(value));
    if(IsByteswapRequired(endian)) {
        switch(sizeof(T)) {
            case 1:
                break;
            case 2:
                value = bswap_16(value);
                break;
            case 4:
            {
                u32 * valuePtr = (u32*)&value;
                u32 value32 = bswap_32(*valuePtr);
                T * targetValuePtr = (T*)&value32;

                value = *targetValuePtr;
            } break;
            case 8:
                value = bswap_64(value);
                break;
            default:
                value = 0;
                break;
        }
    }

    return value;
}


namespace SIM::GX {

    class VertexDecoderCRC_c0846887: public IVertexDecoder {
    public:
        VertexDecoderCRC_c0846887() {};
        virtual void DecodeVerts(u8 * byteStream, RenderVertex * vertsOut, size_t numVerts, std::endian endian);
        virtual ~VertexDecoderCRC_c0846887() {};
    private:
        struct BinaryVertex {
            u8 position;
            u8 color0;
        };
        static constexpr int BytesPerVertex = sizeof(BinaryVertex);
    };
    void VertexDecoderCRC_c0846887::DecodeVerts(u8 * byteStream, RenderVertex * vertsOut, size_t numVerts, std::endian endian){
        const BinaryVertex * vertsIn = (const BinaryVertex * )(byteStream);
        auto& gxState = GetGlobalState();
        const auto& positionArray = gxState.GetVertexArray(static_cast<GXAttr>(9));
        const auto& color0Array = gxState.GetVertexArray(static_cast<GXAttr>(11));
        const u8 * arrayCursor;
        for(size_t i=0; i < numVerts; i++) {
            arrayCursor = (u8*)(positionArray.mArrayPtr) + (positionArray.mStride * vertsIn[i].position);
            vertsOut[i].position.coords[0] = ReadUnaligned<s16>(arrayCursor + (sizeof(s16) * 0), endian);
            vertsOut[i].position.coords[1] = ReadUnaligned<s16>(arrayCursor + (sizeof(s16) * 1), endian);
            vertsOut[i].position.coords[2] = ReadUnaligned<s16>(arrayCursor + (sizeof(s16) * 2), endian);
            arrayCursor = (u8*)(color0Array.mArrayPtr) + (color0Array.mStride * vertsIn[i].color0);
            vertsOut[i].color0.values[0] = ReadUnaligned<u8>(arrayCursor + (sizeof(u8) * 0), endian);
            vertsOut[i].color0.values[1] = ReadUnaligned<u8>(arrayCursor + (sizeof(u8) * 1), endian);
            vertsOut[i].color0.values[2] = ReadUnaligned<u8>(arrayCursor + (sizeof(u8) * 2), endian);
            vertsOut[i].color0.values[3] = ReadUnaligned<u8>(arrayCursor + (sizeof(u8) * 3), endian);
        }
    }
    std::shared_ptr<IVertexDecoder> GetVertexDecoder(u32 crc) {
        switch(crc) {
            case 0xc0846887:
                return std::make_shared<VertexDecoderCRC_c0846887>();
            default:
                return nullptr;
        }
    }
}
