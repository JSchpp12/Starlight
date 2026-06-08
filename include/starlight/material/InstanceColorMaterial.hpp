#pragma once

#include "StarMaterial.hpp"
#include "starlight/material/IColorProvider.hpp"

namespace star::material
{
class InstanceColorMaterial : public StarMaterial
{
  public:
    InstanceColorMaterial(std::unique_ptr<IColorProvider> colorProvider)
        : StarMaterial(), m_colorProvider(std::move(colorProvider)) {};

    void addDescriptorSetLayoutsTo(star::StarDescriptorSetLayout::Builder &constBuilder) const override;

  protected:
    std::unique_ptr<StarShaderInfo> buildShaderInfo(core::device::DeviceContext &context,
                                                    const uint8_t &numFramesInFlight,
                                                    StarShaderInfo::Builder builder) override;

    std::unique_ptr<IColorProvider> m_colorProvider;
};
} // namespace star::material