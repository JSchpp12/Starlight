#include "starlight/material/InstanceColorMaterial.hpp"

#include "core/helper/queue/QueueHelpers.hpp"
#include "starlight/common/buffers/TransferRequest_InstanceColorInfo.hpp"

namespace star::material
{
void InstanceColorMaterial::addDescriptorSetLayoutsTo(star::StarDescriptorSetLayout::Builder &constBuilder) const
{
    constBuilder.addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);
}

std::unique_ptr<StarShaderInfo> InstanceColorMaterial::buildShaderInfo(core::device::DeviceContext &context,
                                                                       const uint8_t &numFramesInFlight,
                                                                       StarShaderInfo::Builder builder)
{
    assert(m_colorProvider);

    auto colors = context.getManagerRenderResource().addRequest(
        context.getDeviceID(),
        std::make_unique<TransferRequest::InstanceColorInfo>(
            m_colorProvider->getColors(),
            core::helper::GetEngineDefaultQueue(context.getEventBus(), context.getGraphicsManagers().queueManager,
                                                star::Queue_Type::Tgraphics)
                ->getParentQueueFamilyIndex()));

    for (size_t i{0}; i < context.frameTracker().getSetup().getNumFramesInFlight(); i++)
    {
        builder.startOnFrameIndex(i);
        builder.startSet();
        builder.add(star::StarShaderInfo::BufferInfo{colors});
    }

    return builder.build();
}
} // namespace star::material