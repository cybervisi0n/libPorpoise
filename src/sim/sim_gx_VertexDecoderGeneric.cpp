#include "simulator/sim_gx_VertexDecoderGeneric.hpp"

#include "dolphin/gx/GXEnum.h"
#include "dolphin/os.h"
#include "simulator/byteswap.h"
#include "simulator/sim_gx_Geometry.hpp"

#include <bit>
#include <cstring>

namespace SIM::GX {

static bool IsByteswapRequired(std::endian endian) {
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
                OSReport("GXGeometry: Warning: tried to byteswap on an unsupported size");
                break;
        }
    }

    return value;
}

static inline size_t ComponentSize(GXCompType type) {
    switch (type) {
        case GX_U8:
        case GX_S8:
            return 1;
        case GX_U16:
        case GX_S16:
            return 2;
        case GX_F32:
            return 4;
        default:
            return 0;
    }
}

void NoOpComponent(const u8 * source, GXCompCnt dummy, GXCompType type, u8 fraction, float * output) {

}

void MatrixIdxComponent(const u8 * source, GXCompCnt dummy, GXCompType type, u8 fraction, float * output) {
    u32 * outputInt = (u32*)output;
    switch (type) {
        case GX_U8:
            *outputInt = ReadUnaligned<u8>(source, std::endian::native);
            break;
        case GX_S8:
            *outputInt = ReadUnaligned<s8>(source, std::endian::native);
            break;
        case GX_U16:
            *outputInt = ReadUnaligned<u16>(source, std::endian::native);
            break;
        case GX_S16:
            *outputInt = ReadUnaligned<s16>(source, std::endian::native);
            break;
        default:
            *outputInt = 0;
            break;
    }
}

void DecodePositionComponent(const u8* source, GXCompCnt dummy, GXCompType type, u8 fraction, float * output) {
    // Actual verts *should* always be in native endian
    switch (type) {
        case GX_U8:
            *output = std::ldexp(static_cast<float>(ReadUnaligned<u8>(source, std::endian::native)), -fraction);
            break;
        case GX_S8:
            *output = std::ldexp(static_cast<float>(ReadUnaligned<s8>(source, std::endian::native)), -fraction);
            break;
        case GX_U16:
            *output = std::ldexp(static_cast<float>(ReadUnaligned<u16>(source, std::endian::native)), -fraction);
            break;
        case GX_S16:
            *output = std::ldexp(static_cast<float>(ReadUnaligned<s16>(source, std::endian::native)), -fraction);
            break;
        case GX_F32:
            *output =  ReadUnaligned<f32>(source, std::endian::native);
            break;
        default:
            *output = 0.0f;
            break;
    }
}

static inline void DecodeColor(const u8* source, GXCompCnt componentCount, GXCompType type, u8 dummyFrac,
                 float *output ) {
    u8 rgba[4] = {255, 255, 255, 255};

    switch (type) {
        case GX_RGB565: {
            const u16 packed = ReadUnaligned<u16>(source, std::endian::native);
            rgba[0] = static_cast<u8>(((packed >> 11) & 0x1f) * 255 / 31);
            rgba[1] = static_cast<u8>(((packed >> 5) & 0x3f) * 255 / 63);
            rgba[2] = static_cast<u8>((packed & 0x1f) * 255 / 31);
            break;
        }
        case GX_RGB8:
        case GX_RGBX8:
            rgba[0] = source[0];
            rgba[1] = source[1];
            rgba[2] = source[2];
            break;
        case GX_RGBA4: {
            const u16 packed = ReadUnaligned<u16>(source, std::endian::native);
            rgba[0] = static_cast<u8>(((packed >> 12) & 0x0f) * 17);
            rgba[1] = static_cast<u8>(((packed >> 8) & 0x0f) * 17);
            rgba[2] = static_cast<u8>(((packed >> 4) & 0x0f) * 17);
            rgba[3] = static_cast<u8>((packed & 0x0f) * 17);
            break;
        }
        case GX_RGBA6: {
            const u32 packed =
                (static_cast<u32>(source[0]) << 16) |
                (static_cast<u32>(source[1]) << 8) |
                static_cast<u32>(source[2]);
            rgba[0] = static_cast<u8>((((packed >> 18) & 0x3f) * 255 + 31) / 63);
            rgba[1] = static_cast<u8>((((packed >> 12) & 0x3f) * 255 + 31) / 63);
            rgba[2] = static_cast<u8>((((packed >> 6) & 0x3f) * 255 + 31) / 63);
            rgba[3] = static_cast<u8>(((packed & 0x3f) * 255 + 31) / 63);
            break;
        }
        case GX_RGBA8:
        default:
            rgba[0] = source[0];
            rgba[1] = source[1];
            rgba[2] = source[2];
            rgba[3] = componentCount == GX_CLR_RGBA ? source[3] : 255;
            break;
    }

    constexpr float byteScale = 1.0f / 255.0f;
    for (size_t i = 0; i < 4; ++i) {
        output[i] = static_cast<float>(rgba[i]) * byteScale;
    }
}

static inline bool ReadArrayIndex(const u8*& cursor, const u8* end, GXAttrType descriptor,
                    size_t& index, std::endian endian) {

    switch(descriptor) {
        case GX_INDEX8:
            if (cursor + sizeof(u8) > end) {
                return false;
            }
            index = *cursor++;
            return true;
        case GX_INDEX16:
            if (cursor + sizeof(u16) > end) {
                return false;
            }
            index = ReadUnaligned<u16>(cursor, endian);
            cursor += sizeof(u16);
            return true;
        default:
            return false;
    }
}


VertexDecoderGeneric::VertexDecoderGeneric(const VertexFormat& mFormat, const GXAttrType * descriptors, size_t bytesPerVertex) :
    mBytesPerVertex(bytesPerVertex),
    mFormat(mFormat) {
    for(int i = 0; i < GX_VA_MAX_ATTR; i++) {
        mDescriptors[i] = descriptors[i];
    }

    #if 1

    static constexpr std::array CompTypeStrings = {
        "GX_U8",
        "GX_S8",
        "GX_U16",
        "GX_S16",
        "GX_F32",
        "GX_RGBA8"
    };

    static constexpr std::array CompCntStrings = {
        "GX_POS_XY",
        "GX_POS_XYZ",
        "GX_NRM_NBT3"
    };
    static constexpr std::array DescriptorStrings = {
        "GX_NONE",
        "GX_DIRECT",
        "GX_INDEX8",
        "GX_INDEX16"
    };
    //print the format/descriptors
    printf("FormatDescriptor(SIM::GX::VertexFormat{{{");
    for(int attrIdx = GX_VA_PNMTXIDX; attrIdx < GX_VA_MAX_ATTR; attrIdx++) {
        auto& attrFmt = mFormat.mAttributes[attrIdx];
        printf("{%s, %s, %d}", CompCntStrings[attrFmt.mComponents], CompTypeStrings[attrFmt.mDataType], attrFmt.mFraction);
        if(attrIdx < GX_VA_MAX_ATTR -1) {
            printf(", ");
        }
    }
    printf("}}}, {");
    for(int attrIdx = GX_VA_PNMTXIDX; attrIdx < GX_VA_MAX_ATTR; attrIdx++) {
        printf("%s", DescriptorStrings[(int)(mDescriptors[attrIdx])]);
        if(attrIdx < GX_VA_MAX_ATTR -1) {
            printf(", ");
        }
    }
    printf("}),\n");
    #endif

    auto& gxState = GetGlobalState();

    auto ProcessAttribute = [this, &gxState, &mFormat](GXAttr attr, size_t (*numComponentsFunc)(GXCompCnt)) mutable -> void {
        auto& curFormat = mFormat.mAttributes[attr];
        mVtxInfo[attr].mDescriptor = mDescriptors[attr];
        mVtxInfo[attr].mComponentCount = numComponentsFunc(curFormat.mComponents);
        if(attr == GX_VA_CLR0 || attr == GX_VA_CLR1) {
             mVtxInfo[attr].mComponentSize = gxState.GetDescriptorSize(GX_DIRECT, curFormat.mDataType, true);
        } else {
            mVtxInfo[attr].mComponentSize = ComponentSize(curFormat.mDataType);
        }
        mVtxInfo[attr].mVertexArray = gxState.GetVertexArray(attr);
    };

    ProcessAttribute(GX_VA_PNMTXIDX, gxState.GetNumMtxIdxComponents);
    for(int i=GX_VA_TEX0MTXIDX; i<=GX_VA_TEX7MTXIDX; i++) {
        ProcessAttribute(static_cast<GXAttr>(i), gxState.GetNumMtxIdxComponents);
    }

    ProcessAttribute(GX_VA_POS, gxState.GetNumPositionComponents);
    ProcessAttribute(GX_VA_NRM, gxState.GetNumNormalComponents);

    ProcessAttribute(GX_VA_CLR0, gxState.GetNumColorComponents);
    ProcessAttribute(GX_VA_CLR1, gxState.GetNumColorComponents);   
    for(int attribNum = GX_VA_TEX0; attribNum <= GX_VA_TEX7; attribNum++) {
        ProcessAttribute(static_cast<GXAttr>(attribNum), gxState.GetNumTexCoordComponents); 
    }

    ProcessAttribute(GX_VA_NBT, gxState.GetNumNBTComponents);
}

void VertexDecoderGeneric::DecodeVerts(u8 * byteStream, RenderVertex * vertsOut, size_t numVerts, std::endian endian) {
    const u8* cursor = byteStream;
    const u8* end = cursor + mBytesPerVertex * numVerts;

    auto BuildRenderVertexAttr = [this, &cursor, &end, endian](GXAttr attr, float * outputArray, void (*decodeComponentFunc)(const u8*, GXCompCnt, GXCompType, u8, float *)) mutable -> void {
        const auto& info = mVtxInfo[attr];
        if(info.mDescriptor == GX_NONE) {
            return;
        }

        const u8* dataSource = nullptr;
        if (info.mDescriptor == GX_DIRECT) {
            const size_t directSize = info.mComponentCount * info.mComponentSize;
            if (cursor + directSize > end) {
                return;
            }
            dataSource = cursor;
            cursor += directSize;
        } else {
            size_t arrayIndex;
            if (!ReadArrayIndex(cursor, end, info.mDescriptor, arrayIndex, endian)) {
                return;
            }
            
            dataSource = static_cast<const u8*>(info.mVertexArray.mArrayPtr) +
                             arrayIndex * static_cast<size_t>(info.mVertexArray.mStride);
        }

        for (size_t component = 0; component < info.mComponentCount; ++component) {
                decodeComponentFunc(
                dataSource + component * info.mComponentSize,
                mFormat.mAttributes[attr].mComponents,
                mFormat.mAttributes[attr].mDataType,
                mFormat.mAttributes[attr].mFraction,
                &outputArray[component]);
        }
    };

    for (size_t vertexIndex = 0; vertexIndex < numVerts; ++vertexIndex) {
        RenderVertex& output = vertsOut[vertexIndex];

        output = {0};

        BuildRenderVertexAttr(GX_VA_PNMTXIDX, (float*)&output.posNormalMtxIdx, MatrixIdxComponent);
        for(int i=GX_VA_TEX0MTXIDX; i <=GX_VA_TEX7MTXIDX; i++) {
            BuildRenderVertexAttr(static_cast<GXAttr>(i), (float*)&output.texMtxIdx[i], MatrixIdxComponent);
        }

        BuildRenderVertexAttr(GX_VA_POS, output.position.coords, DecodePositionComponent);
        BuildRenderVertexAttr(GX_VA_NRM, output.normal.coords, DecodePositionComponent);

        BuildRenderVertexAttr(GX_VA_CLR0, output.color0.values, DecodeColor);
        BuildRenderVertexAttr(GX_VA_CLR1, nullptr, NoOpComponent);

        for(int attribNum = GX_VA_TEX0; attribNum <= GX_VA_TEX7; attribNum++) {
            BuildRenderVertexAttr(static_cast<GXAttr>(attribNum), output.texCoords[attribNum - GX_VA_TEX0].coords, DecodePositionComponent);
        }

        BuildRenderVertexAttr(GX_VA_NBT, nullptr, NoOpComponent);

    }
}

}