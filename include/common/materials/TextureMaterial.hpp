#pragma once

#include "Handle.hpp"
#include "StarMaterial.hpp"
#include "core/device/DeviceID.hpp"

namespace star
{
class TextureMaterial : public StarMaterial
{
  public:
    TextureMaterial(std::string texturePath) : m_texturePath(std::move(texturePath))
    {
    }

    TextureMaterial(std::string texturePath, const glm::vec4 &surfaceColor, const glm::vec4 &highlightColor,
                    const glm::vec4 &ambient, const glm::vec4 &diffuse, const glm::vec4 &specular, const int &shiny)
        : StarMaterial(surfaceColor, highlightColor, ambient, diffuse, specular, shiny),
          m_texturePath(std::move(texturePath))
    {
    }
    virtual ~TextureMaterial() = default;

    virtual void prepRender(core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                            star::StarShaderInfo::Builder frameBuilder) override;

    virtual std::vector<std::pair<vk::DescriptorType, const int>> getDescriptorRequests(
        const int &numFramesInFlight) const override;

    virtual void addDescriptorSetLayoutsTo(StarDescriptorSetLayout::Builder &builder) const override;

  protected:
    std::string m_texturePath = "";
    Handle m_textureHandle = Handle();

    virtual std::unique_ptr<StarShaderInfo> buildShaderInfo(core::device::DeviceContext &context, const uint8_t &numFramesInFlight, 
      StarShaderInfo::Builder builder); 
};
} // namespace star