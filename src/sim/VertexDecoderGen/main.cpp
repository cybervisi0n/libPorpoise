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
    }

    int components = GetNumComponents(attr, compCnt);
    if(components > 1) {
        arrayTypeString = std::format("[{}]", components);
    }

    return baseTypeString + arrayTypeString;
}

static std::string GenerateBinaryVertexStruct(const FormatDescriptor& fmtDesc) {
    std::string ret = "struct BinaryVertex {\n";
    const auto& fmt = fmtDesc.first;
    const auto& descriptors = fmtDesc.second;
    for(int attrIdx = GX_VA_PNMTXIDX; attrIdx < GX_VA_MAX_ATTR; attrIdx++) {
        if(descriptors[attrIdx] != GX_NONE) {
            ret += std::format("    {} {};\n", 
                GetTypeString(static_cast<GXAttr>(attrIdx), descriptors[attrIdx], fmt.mAttributes[attrIdx].mDataType, fmt.mAttributes[attrIdx].mComponents),
                VertexAttributeStrings[attrIdx]
            );
        }
    }

    ret += "};\n";
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
    ret += "virtual ~" + className + "();\n";
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
    ret += "auto& gxState = GetGlobalState();\n";

    for(int attrIdx = GX_VA_PNMTXIDX; attrIdx < GX_VA_MAX_ATTR; attrIdx++) {
        if(descriptors[attrIdx] == GX_INDEX8 || descriptors[attrIdx] == GX_INDEX16) {
            ret += std::format("const auto& {}Array = gxState.GetVertexArray(static_cast<GXAttr>({}));\n", VertexAttributeStrings[attrIdx], attrIdx);
        }
    }

    ret += "for(size_t i=0; i < numVerts; i++) {\n";

    for(int attrIdx = GX_VA_PNMTXIDX; attrIdx < GX_VA_MAX_ATTR; attrIdx++) {
        if(descriptors[attrIdx] == GX_NONE) {
            continue;
        }

        int numComponents = GetNumComponents(static_cast<GXAttr>(attrIdx), fmt.mAttributes[attrIdx].mComponents);
        std::string sourceString = "";
        if(descriptors[attrIdx] == GX_DIRECT) {

        } else if(descriptors[attrIdx] == GX_INDEX8 || descriptors[attrIdx] == GX_INDEX16) {

        }

        for(int i=0; i < numComponents; i++) {
            std::string arrayDestStr = "";
            if(numComponents > 1 || (attrIdx >= GX_VA_TEX0 && attrIdx <= GX_VA_TEX7)) {
                arrayDestStr = std::format("[{}]", i);
            }

            ret += std::format("vertsOut[i].{}{} = {}\n", RenderVertexAttrStrings[attrIdx], arrayDestStr, sourceString);
        }
    }

    ret += "}\n";
    ret += "}\n";
    return ret;
}

int main(int argc, char ** argv) {

    int decoderCount = GetDecoderTableCount();
    const auto * decoderTable = GetDecoderTable();




    std::string ret = "";

    // add includes
    ret += "#include <dolphin/types.h>\n";
    ret += "#include \"simulator/sim_gx_IVertexDecoder.hpp\"\n";

    ret += "namespace SIM::GX {";

    std::string classDefs = "";
    std::string implementations = "";

    for(int i = 0; i < decoderCount; i++) {
        auto& fmt = decoderTable[i].first;
        auto& descriptors = decoderTable[i].second;

        u32 crcVal = SIM_crc32buf((const u8*)(&fmt), sizeof(SIM::GX::VertexFormat));
        crcVal = SIM_updateCRC32buf(crcVal, (const u8*)(descriptors.data()), sizeof(GXAttrType) * GX_VA_MAX_ATTR);
        classDefs += GenerateClassDef(decoderTable[i], crcVal);
        implementations += GenerateDecodeFunc(decoderTable[i], crcVal);

    }

    ret += classDefs;
    ret += implementations;

    ret += "}\n";
    printf("%s\n",  ret.c_str());

    return 0;
}