#ifndef DECODER_TABLE_HPP
#define DECODER_TABLE_HPP

#include "simulator/sim_gx_State.hpp"
#include <dolphin/types.h>
#include <dolphin/gx/GXEnum.h>
#include <array>
#include <utility>

using FormatDescriptor = std::pair<SIM::GX::VertexFormat, std::array<GXAttrType, GX_VA_MAX_ATTR>>;

const FormatDescriptor * GetDecoderTable();
int GetDecoderTableCount();



#endif