#pragma once

#include "TextureMaterial.hpp"

#include "Handle.hpp"

#include <glm/glm.hpp>

namespace star
{
class BumpMaterial : public TextureMaterial
{
  public:
    BumpMaterial(std::string bumpMapFilePath, std::string baseColorFilePath)
        : TextureMaterial(std::move(baseColorFilePath)), m_bumpMapFilePath(std::move(bumpMapFilePath))
    {
    }
    BumpMaterial(std::string bumpMapFilePath, std::string baseColorFilePath, const glm::vec4 &surfaceColor,
                 const glm::vec4 &highlightColor, const glm::vec4 &ambient, const glm::vec4 &diffuse,
                 const glm::vec4 &specular, const int &shiny)
        : TextureMaterial(std::move(baseColorFilePath), surfaceColor, highlightColor, ambient, diffuse, specular,
                          shiny),
          m_bumpMapFilePath(std::move(bumpMapFilePath)) {};

    virtual ~BumpMaterial() = default;

    virtual void prepRender(core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                            star::StarShaderInfo::Builder frameBuilder) override;

    virtual void addDescriptorSetLayoutsTo(star::StarDescriptorSetLayout::Builder &constBuilder) const override;

    virtual std::vector<std::pair<vk::DescriptorType, const int>> getDescriptorRequests(
        const int &numFramesInFlight) const override;

  protected:
    std::string m_bumpMapFilePath = "";
    Handle m_bumpMap = Handle();

    virtual std::unique_ptr<StarShaderInfo> buildShaderInfo(core::device::DeviceContext &context,
                                                            const uint8_t &numFramesInFlight,
                                                            StarShaderInfo::Builder builder) override;
};
} // namespace star