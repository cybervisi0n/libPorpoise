#include <dolphin/types.h>
#include <dolphin/gx/GXEnum.h>

#include "simulator/sim_crc32.h"
#include "simulator/sim_gx_State.hpp"

#include "DecoderTable.hpp"

#include <cstdio>
#include <format>
#include <string>

static constexpr std::array VertexAttributeStrings = {
    "posNormalMtxIdx",
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

static constexpr std::array CompTypeStrings = {
    "u8",
    "s8",
    "u16",
    "s16",
    "f32"
};

static constexpr std::array ColorCompTypeStrings = {
    "u16",
    "u8",
    "u8",
    "u16",
    "u8[3]",
    "u8"
};

static constexpr std::array RenderVertexAttrStrings = {
    "posNormalMtxIdx",
    "texMtxIdx[0]",
    "texMtxIdx[1]",
    "texMtxIdx[2]",
    "texMtxIdx[3]",
    "texMtxIdx[4]",
    "texMtxIdx[5]",
    "texMtxIdx[6]",
    "texMtxIdx[7]",
    "position.coords",
    "normal.coords",
    "color0.values",
    "color1.values",
    "texCoords[0].coords",
    "texCoords[1].coords",
    "texCoords[2].coords",
    "texCoords[3].coords",
    "texCoords[4].coords",
    "texCoords[5].coords",
    "texCoords[6].coords",
    "texCoords[7].coords",
    "",
    "",
    "",
    "",
    ""
};

static int GetNumComponents(GXAttr attr, GXCompCnt cnt) {
    if(attr == GX_VA_POS) {
        return cnt == GX_POS_XY ? 2 : 3;
    }

    if(attr == GX_VA_NRM) {
        return 3;
    }
    
    if(attr == GX_VA_CLR0 || attr == GX_VA_CLR1) {
        return cnt == GX_CLR_RGB ? 3 : 4;
    }

    if(attr >= GX_VA_TEX0 && attr <= GX_VA_TEX7) {
        return cnt == GX_TEX_S ? 1 : 2;
    }

    return 1;
}

static std::string GetTypeString(GXAttr attr, GXAttrType descriptor, GXCompType compType, GXCompCnt compCnt) {
    if(descriptor == GX_INDEX8) {
        return "u8";
    }

    if(descriptor == GX_INDEX16) {
        return "u16";
    }

    // GX direct
    std::string baseTypeString = "";
    std::string arrayTypeString = "";
    if(attr != GX_VA_CLR0 && attr != GX_VA_CLR1) {
        baseTypeString = CompTypeStrings[compType];
    } else {
        baseTypeString = ColorCompTypeStrings[compType];
    }

    return baseTypeString;
}

static std::string GetArrayTypeString(GXAttr attr, GXAttrType descriptor, GXCompType compType, GXCompCnt compCnt) {
    std::string arrayTypeString = "";
    if(descriptor == GX_INDEX8 || descriptor == GX_INDEX16) {
        return arrayTypeString;
    }

    int components = GetNumComponents(attr, compCnt);
    if(components > 1) {
        arrayTypeString = std::format("[{}]", components);
    }   

    return arrayTypeString;
}



static std::string GenerateBinaryVertexStruct(const FormatDescriptor& fmtDesc) {
    std::string ret = "struct BinaryVertex {\n";
    const auto& fmt = fmtDesc.first;
    const auto& descriptors = fmtDesc.second;
    for(int attrIdx = GX_VA_PNMTXIDX; attrIdx < GX_VA_MAX_ATTR; attrIdx++) {
        if(descriptors[attrIdx] != GX_NONE) {
            ret += std::format("    {} {}{};\n", 
                GetTypeString(static_cast<GXAttr>(attrIdx), descriptors[attrIdx], fmt.mAttributes[attrIdx].mDataType, fmt.mAttributes[attrIdx].mComponents),
                VertexAttributeStrings[attrIdx],
                GetArrayTypeString(static_cast<GXAttr>(attrIdx), descriptors[attrIdx], fmt.mAttributes[attrIdx].mDataType, fmt.mAttributes[attrIdx].mComponents)
            );
        }
    }

    ret += "}__attribute__((packed));\n";
    return ret;
}

static std::string GetClassName(u32 crc) {
    return std::format("VertexDecoderCRC_{:08x}", crc);
}

static std::string GenerateClassDef(const FormatDescriptor& fmtDesc, u32 crc) {
    std::string className = GetClassName(crc);

    std::string ret = "class " + className + ": public IVertexDecoder {\n";
    ret += "public:\n";
    ret += className + "() {};\n";
    ret += "virtual void DecodeVerts(u8 * byteStream, RenderVertex * vertsOut, size_t numVerts, std::endian endian);\n";
    ret += "virtual ~" + className + "() {};\n";
    ret += "private:\n";
    ret += GenerateBinaryVertexStruct(fmtDesc);
    ret += "static constexpr int BytesPerVertex = sizeof(BinaryVertex);\n";
    ret += "};\n";

    return ret;
}

static std::string GenerateDecodeFunc(const FormatDescriptor& fmtDesc, u32 crc) {
    std::string className = GetClassName(crc);
    const auto& fmt = fmtDesc.first;
    const auto& descriptors = fmtDesc.second;

    std::string ret = "";

    ret += std::format("void {}::DecodeVerts(u8 * byteStream, RenderVertex * vertsOut, size_t numVerts, std::endian endian)", className);
    ret += "{\n";
    ret += "const BinaryVertex * vertsIn = (const BinaryVertex * )(byteStream);\n";
    ret += "[[maybe_unused]] auto& gxState = GetGlobalState();\n";

    // If any attribute is indexed, we need an extra pointer variable to use for the arrays
    bool indexedAttr = false;

    for(int attrIdx = GX_VA_PNMTXIDX; attrIdx < GX_VA_MAX_ATTR; attrIdx++) {
        if(descriptors[attrIdx] == GX_INDEX8 || descriptors[attrIdx] == GX_INDEX16) {
            ret += std::format("const auto& {}Array = gxState.GetVertexArray(static_cast<GXAttr>({}));\n", VertexAttributeStrings[attrIdx], attrIdx);
            indexedAttr = true;
        }
    }

    if(indexedAttr) {
        ret += "const u8 * arrayCursor;\n";
    }

    ret += "for(size_t i=0; i < numVerts; i++) {\n";

    for(int attrIdx = GX_VA_PNMTXIDX; attrIdx < GX_VA_MAX_ATTR; attrIdx++) {
        if(descriptors[attrIdx] == GX_NONE) {
            continue;
        }

        int numComponents = GetNumComponents(static_cast<GXAttr>(attrIdx), fmt.mAttributes[attrIdx].mComponents);
        std::string baseSourceString;
        std::string baseTypeString;
        if(attrIdx != GX_VA_CLR0 && attrIdx != GX_VA_CLR1) {
            baseTypeString = CompTypeStrings[fmt.mAttributes[attrIdx].mDataType];
        } else {
            baseTypeString = ColorCompTypeStrings[fmt.mAttributes[attrIdx].mDataType];
        }

        baseSourceString = std::format("ReadUnaligned<{}>", baseTypeString);
        if(descriptors[attrIdx] == GX_DIRECT) {
            baseSourceString += std::format("((const u8*)&(vertsIn[i].{}", VertexAttributeStrings[attrIdx]);
        } else if(descriptors[attrIdx] == GX_INDEX16) {
            ret += "arrayCursor = endian == std::endian::native ? ";
            ret += std::format("((u8*)({}Array.mArrayPtr) + ({}Array.mStride * vertsIn[i].{})):\n", VertexAttributeStrings[attrIdx], VertexAttributeStrings[attrIdx], VertexAttributeStrings[attrIdx]);
            ret += std::format("((u8*)({}Array.mArrayPtr) + ({}Array.mStride * bswap_16(vertsIn[i].{})));\n", VertexAttributeStrings[attrIdx], VertexAttributeStrings[attrIdx], VertexAttributeStrings[attrIdx]);
            baseSourceString += "(arrayCursor";
        } else if(descriptors[attrIdx] == GX_INDEX8) {
            ret += std::format("arrayCursor = ((u8*)({}Array.mArrayPtr) + ({}Array.mStride * vertsIn[i].{}));\n", VertexAttributeStrings[attrIdx], VertexAttributeStrings[attrIdx], VertexAttributeStrings[attrIdx]);
            baseSourceString += "(arrayCursor";
        }

        for(int i=0; i < numComponents; i++) {
            std::string arrayDestStr = "";
            std::string sourceString = baseSourceString;
            if(numComponents > 1 || (attrIdx >= GX_VA_TEX0 && attrIdx <= GX_VA_TEX7)) {
                arrayDestStr = std::format("[{}]", i);
                if(descriptors[attrIdx] == GX_DIRECT) {
                    sourceString += arrayDestStr;
                    sourceString += ")";
                }
            }
            if(numComponents == 1 && descriptors[attrIdx] == GX_DIRECT) {
                sourceString += ")";
            }
            
            if((descriptors[attrIdx] == GX_INDEX8) || (descriptors[attrIdx] == GX_INDEX16)) {
                sourceString += std::format(" + (sizeof({}) * {})", baseTypeString, i);
            }

            sourceString += ", std::endian::native)";

            if(fmt.mAttributes[attrIdx].mFraction != 0) {
                sourceString = "std::ldexp(static_cast<float>(" + sourceString + "), -" + std::to_string(fmt.mAttributes[attrIdx].mFraction) + ")";
            }

            if(attrIdx == GX_VA_CLR0 || attrIdx == GX_VA_CLR1) {
                sourceString += " / 255.0f";
            }



            ret += std::format("vertsOut[i].{}{} = {};\n", RenderVertexAttrStrings[attrIdx], arrayDestStr, sourceString);
        }
    }

    ret += "}\n";
    ret += "}\n";
    return ret;
}

static std::string GenerateSwitchCase(u32 crc) {
    std::string ret = "";
    ret += std::format("case 0x{:08x}:\n", crc);
    ret += std::format("return std::make_shared<{}>();\n", GetClassName(crc));
    return ret;
}

int main(int argc, char ** argv) {

    int decoderCount = GetDecoderTableCount();
    const auto * decoderTable = GetDecoderTable();

    if(argc < 2) {
        printf("Usage: VertexDecoderGen {outputFilePath}\n");
        return -1;
    }




    std::string ret = "";

    // add includes
    ret += "#include <dolphin/types.h>\n";
    ret += "#include \"simulator/sim_gx_Geometry.hpp\"\n";
    ret += "#include \"simulator/sim_gx_IVertexDecoder.hpp\"\n";
    ret += "#include \"simulator/sim_gx_State.hpp\"\n";
    ret += "#include \"simulator/byteswap.h\"\n";
    ret += "#include <memory>\n";
    ret += "#include <cstring>\n";

// Add some common functions
    ret += R""(
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
)"";
    ret += "\n\n";

    ret += "namespace SIM::GX {\n\n";

    std::string classDefs = "";
    std::string implementations = "";
    std::string switchCases = "";

    for(int i = 0; i < decoderCount; i++) {
        auto& fmt = decoderTable[i].first;
        auto& descriptors = decoderTable[i].second;

        u32 crcVal = SIM_crc32buf((const u8*)(&fmt), sizeof(SIM::GX::VertexFormat));
        crcVal = SIM_updateCRC32buf(crcVal, (const u8*)(descriptors.data()), sizeof(GXAttrType) * GX_VA_MAX_ATTR);
        classDefs += GenerateClassDef(decoderTable[i], crcVal);
        implementations += GenerateDecodeFunc(decoderTable[i], crcVal);
        switchCases += GenerateSwitchCase(crcVal);

    }

    ret += classDefs;
    ret += implementations;

    ret += "std::shared_ptr<IVertexDecoder> GetVertexDecoder(u32 crc) {\n";
    ret += "switch(crc) {\n";
    ret += switchCases;
    ret += "default:\n";
    ret += "return nullptr;\n";
    ret += "}\n";
    ret += "}\n";

    ret += "}\n";

    FILE * outputFile = fopen(argv[1], "w");

    fprintf(outputFile, "%s", ret.c_str());
    fclose(outputFile);

    return 0;
}