#pragma once

#include "starlight/material/IMaterialProvider.hpp"

#include <cassert>
#include <memory>
#include <utility>
#include <vector>

namespace star
{
/// @brief Default material provider: wraps an already constructed material until make() hands it to the manager.
/// @details Metadata is delegated to the wrapped material, so the provider cannot drift from the instance.
/// The material object is constructed by the maker as before; only its manager record and resource work are
/// deferred to prepRender.
class MaterialProvider final : public IMaterialProvider
{
  public:
    explicit MaterialProvider(std::unique_ptr<StarMaterial> material, std::vector<setup::TransferWait> waits = {},
                              setup::RenderResourceSetup::PrepRequest prep = {},
                              std::vector<ReadinessCondition> conditions = {})
        : m_material(std::move(material)), m_waits(std::move(waits)), m_prep(std::move(prep)),
          m_conditions(std::move(conditions))
    {
        assert(m_material != nullptr);
    }

    void addDescriptorSetLayoutsTo(StarDescriptorSetLayout::Builder &builder) const override
    {
        assert(m_material != nullptr && "MaterialProvider metadata used after make()");
        m_material->addDescriptorSetLayoutsTo(builder);
    }

    std::vector<std::pair<vk::DescriptorType, const int>> getDescriptorRequests(int numFramesInFlight) const override
    {
        assert(m_material != nullptr && "MaterialProvider metadata used after make()");
        return m_material->getDescriptorRequests(numFramesInFlight);
    }

    [[nodiscard]] Made make(core::device::DeviceContext &) override
    {
        assert(m_material != nullptr && "MaterialProvider::make called more than once");

        Made made;
        made.material = std::move(m_material);
        made.waits = std::move(m_waits);
        made.prep = std::move(m_prep);
        made.conditions = std::move(m_conditions);
        return made;
    }

  private:
    std::unique_ptr<StarMaterial> m_material;
    std::vector<setup::TransferWait> m_waits;
    setup::RenderResourceSetup::PrepRequest m_prep;
    std::vector<ReadinessCondition> m_conditions;
};
} // namespace star
