#pragma once

#include "starlight/setup/RenderResourceSetup.hpp"
#include "starlight/virtual/StarMaterial.hpp"

#include <vulkan/vulkan.hpp>

#include <memory>
#include <utility>
#include <vector>

namespace star
{
/// @brief The material recipe held by an object: type-level metadata plus the deferred instance factory.
/// @details The metadata is needed before an instance exists (phase build and descriptor-pool sizing). `make`
/// runs at prepRender, when the pass's command buffer is known, and returns the material plus any one-shot work.
class IMaterialProvider
{
  public:
    struct Made
    {
        std::unique_ptr<StarMaterial> material;
        std::vector<setup::TransferWait> waits;
        setup::RenderResourceSetup::PrepRequest prep;
    };

    virtual ~IMaterialProvider() = default;

    /// @brief Add this material's descriptor bindings. Must match the material returned by make().
    virtual void addDescriptorSetLayoutsTo(StarDescriptorSetLayout::Builder &builder) const = 0;

    /// @brief Descriptor counts for pool sizing. Must match the material returned by make().
    virtual std::vector<std::pair<vk::DescriptorType, const int>> getDescriptorRequests(
        int numFramesInFlight) const = 0;

    /// @brief Create the material and its one-shot resource work.
    [[nodiscard]] virtual Made make(core::device::DeviceContext &context) = 0;
};
} // namespace star
