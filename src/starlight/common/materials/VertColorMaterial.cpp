#include "VertColorMaterial.hpp"

void star::VertColorMaterial::addDescriptorSetLayoutsTo(star::StarDescriptorSetLayout::Builder& constBuilder) const
{
}

std::unique_ptr<star::StarShaderInfo> star::VertColorMaterial::buildShaderInfo(core::device::DeviceContext &context,
                                               const uint8_t &numFramesInFlight, StarShaderInfo::Builder builder){
    return builder.build(); 
}