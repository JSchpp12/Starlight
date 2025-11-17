#include "core/device/managers/Pipeline.hpp"

#include "core/device/system/event/ShaderCompiled.hpp"

#include <starlight/common/HandleTypeRegistry.hpp>

#include <cassert>

namespace star::core::device::manager
{

void Pipeline::init(std::shared_ptr<device::StarDevice> device, core::device::system::EventBus &bus)
{
    m_shaderBuiltCallbackEventType =
        common::HandleTypeRegistry::instance().registerType(system::event::ShaderCompiledTypeName());
}

void Pipeline::cleanup(core::device::system::EventBus &bus)
{
    this->TaskCreatedResourceManager::cleanup(bus);
    for (const auto &subscriberInfo : m_subscriberShaderBuildInfo)
    {
        if (subscriberInfo.second.isInitialized())
        {
            bus.unsubscribe(
                common::HandleTypeRegistry::instance().getType(system::event::ShaderCompiledTypeName()).value(),
                subscriberInfo.second);
        }
    }
}

void Pipeline::submitTask(device::StarDevice &device, const Handle &handle, job::TaskManager &taskSystem,
                          system::EventBus &eventBus, PipelineRecord *storedRecord)
{
    m_subscriberShaderBuildInfo.insert(std::make_pair(handle, Handle()));

    eventBus.subscribe(m_shaderBuiltCallbackEventType,
                       {[this, handle](const star::common::IEvent &e, bool &keepAlive) {
                            const auto &event = static_cast<const system::event::ShaderCompiled &>(e);

                            auto *record = this->get(handle);
                            bool shouldKeepAlive = true;

                            for (const auto &shader : record->request.pipeline.getShaders())
                            {
                                if (shader.isSameElementAs(event.shaderHandle))
                                {
                                    record->numCompiled++;
                                    if (record->numCompiled == record->request.pipeline.getShaders().size())
                                    {
                                        shouldKeepAlive = false;
                                    }
                                }
                            }

                            keepAlive = shouldKeepAlive;
                        },
                        [this, handle]() -> Handle * {
                            assert(this->m_subscriberShaderBuildInfo.contains(handle));
                            return &this->m_subscriberShaderBuildInfo.at(handle);
                        },
                        [this](const Handle &noLongerNeededHandle) {
                            this->m_subscriberShaderBuildInfo.erase(noLongerNeededHandle);
                        }});
}
} // namespace star::core::device::manager