#pragma once

namespace star::core::renderer
{
struct OnDescriptorPoolReadyWaiter
{
    std::vector<StarRenderGroup> *renderGroups{nullptr};
    star::core::device::DeviceContext *context{nullptr};

    int operator()()
    {
        auto rendererDescriptors = manualCreateDescriptors(c, c.getFrameTracker().getSetup().getNumFramesInFlight());

        for (size_t i{0}; i < renderGroups->size(); i++)
        {
            renderGroups->at(i).onDescriptorPoolReady(*context, )
        }
        return 0;
    }

  private:
    star::StarShaderInfo::Builder createDescriptorBuilder()
    {
        assert(context != nullptr);
        assert(m_infoManagerCamera &&
               "Camera info does not always need to exist. But it should. Hitting this means a change is needed");

        StarDescriptorPool *defaultPool{nullptr};
        {
            const Handle dHandle{.type = common::HandleTypeRegistry::instance().getTypeGuaranteedExist(
                                     core::device::manager::GetDescriptorPoolTypeName),
                                 .id = 0};

            defaultPool = context->getDescriptorPoolManager().get(dHandle)->pool.get();
        }

        assert(defaultPool != nullptr &&
               "Pool has not been created yet. Descriptor pools are created after engine prep phase is complete");

        this->globalSetLayout = createGlobalDescriptorSetLayout(context);
        auto globalBuilder =
            StarShaderInfo::Builder(context.getDeviceID(), context.getDevice(), *defaultPool, numFramesInFlight)
                .addSetLayout(this->globalSetLayout);
        for (int i = 0; i < numFramesInFlight; i++)
        {
            const auto &lightInfoHandle = m_infoManagerLightData->getHandle(i);
            const auto &lightListHandle = m_infoManagerLightList->getHandle(i);
            const auto &cameraHandle = m_infoManagerCamera->getHandle(i);

            globalBuilder.startOnFrameIndex(i)
                .startSet()
                .add(cameraHandle, &context.getManagerRenderResource()
                                        .get<StarBuffers::Buffer>(context.getDeviceID(), cameraHandle)
                                        ->resourceSemaphore)
                .add(lightInfoHandle, &context.getManagerRenderResource()
                                           .get<StarBuffers::Buffer>(context.getDeviceID(), lightInfoHandle)
                                           ->resourceSemaphore)
                .add(lightListHandle, &context.getManagerRenderResource()
                                           .get<StarBuffers::Buffer>(context.getDeviceID(), lightListHandle)
                                           ->resourceSemaphore);
        }

        return globalBuilder;
    }

    star::StarDescriptorSetLayout createSetLayout()
    {
        return StarDescriptorSetLayout::Builder()
            .addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eAll)
            .addBinding(1, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eAll)
            .addBinding(2, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eAll)
            .build();
    }
};
}