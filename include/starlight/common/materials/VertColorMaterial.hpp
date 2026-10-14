#pragma once

#include "StarDescriptorBuilders.hpp"
#include "StarMaterial.hpp"

namespace star
{
class VertColorMaterial : public StarMaterial
{
  public:
    VertColorMaterial() = default;

    VertColorMaterial(glm::vec4 surfaceColor, glm::vec4 highlightColor, glm::vec4 ambient, glm::vec4 diffuse,
                      glm::vec4 specular, int shiny)
        : StarMaterial(surfaceColor, highlightColor, ambient, diffuse, specular, shiny) {};

    void addDescriptorSetLayoutsTo(star::StarDescriptorSetLayout::Builder &constBuilder) const override;

  protected:
    std::unique_ptr<StarShaderInfo> buildShaderInfo(core::device::DeviceContext &context, uint8_t numFramesInFlight,
                                                    StarShaderInfo::Builder builder) override;
};
} // namespace star