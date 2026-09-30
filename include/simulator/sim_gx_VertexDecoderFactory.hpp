#ifndef LIBPORPOISE_SIM_GX_VERTEXDECODERFACTORY_HPP
#define LIBPORPOISE_SIM_GX_VERTEXDECODERFACTORY_HPP

#include "simulator/sim_gx_IVertexDecoder.hpp"
#include <memory>

namespace SIM::GX {

std::shared_ptr<IVertexDecoder> GetVertexDecoder(u32 crc);

}

#endif
