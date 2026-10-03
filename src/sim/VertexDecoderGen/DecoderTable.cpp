#include "DecoderTable.hpp"

#include <dolphin/gx/GXEnum.h>

// Define vertex format/descriptor combinations to be precompiled here
static constexpr std::array DecoderTable = {
    FormatDescriptor(SIM::GX::VertexFormat{{{{GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XYZ, GX_S16, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XYZ, GX_RGBA8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}}}}, {GX_NONE, GX_NONE, GX_NONE, GX_NONE, GX_NONE, GX_NONE, GX_NONE, GX_NONE, GX_NONE, GX_INDEX8, GX_NONE, GX_INDEX8, GX_NONE, GX_NONE, GX_NONE, GX_NONE, GX_NONE, GX_NONE, GX_NONE, GX_NONE, GX_NONE, GX_NONE, GX_NONE, GX_NONE, GX_NONE, GX_NONE}),
    FormatDescriptor(SIM::GX::VertexFormat{{{{GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XYZ, GX_F32, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XYZ, GX_RGBA8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XYZ, GX_F32, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}}}}, {GX_NONE, GX_NONE, GX_NONE, GX_NONE, GX_NONE, GX_NONE, GX_NONE, GX_NONE, GX_NONE, GX_DIRECT, GX_NONE, GX_DIRECT, GX_NONE, GX_DIRECT, GX_NONE, GX_NONE, GX_NONE, GX_NONE, GX_NONE, GX_NONE, GX_NONE, GX_NONE, GX_NONE, GX_NONE, GX_NONE, GX_NONE}),
    FormatDescriptor(SIM::GX::VertexFormat{{{{GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XYZ, GX_F32, 0}, {GX_POS_XY, GX_F32, 0}, {GX_POS_XYZ, GX_RGBA8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XYZ, GX_F32, 0}, {GX_POS_XYZ, GX_F32, 0}, {GX_POS_XYZ, GX_F32, 0}, {GX_POS_XYZ, GX_F32, 0}, {GX_POS_XYZ, GX_F32, 0}, {GX_POS_XYZ, GX_F32, 0}, {GX_POS_XYZ, GX_F32, 0}, {GX_POS_XYZ, GX_F32, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}, {GX_POS_XY, GX_U8, 0}}}}, {GX_DIRECT, GX_NONE, GX_NONE, GX_NONE, GX_NONE, GX_NONE, GX_NONE, GX_NONE, GX_NONE, GX_INDEX16, GX_INDEX16, GX_INDEX16, GX_NONE, GX_INDEX16, GX_INDEX16, GX_NONE, GX_NONE, GX_NONE, GX_NONE, GX_NONE, GX_NONE, GX_NONE, GX_NONE, GX_NONE, GX_NONE, GX_NONE}),
};

const FormatDescriptor * GetDecoderTable() {
    return DecoderTable.data();
}

int GetDecoderTableCount() {
    return DecoderTable.size();
}