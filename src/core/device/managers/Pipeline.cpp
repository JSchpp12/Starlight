#include "core/device/managers/Pipeline.hpp"

#include "core/device/system/event/ShaderCompiled.hpp"

#include <cassert>

namespace star::core::device::manager
{
void Pipeline::cleanup(core::device::system::EventBus &bus)
{
    this->TaskCreatedResourceManager::cleanup(bus);
    for (const auto &handle : m_subscriberShaderBuildInfo)
    {
        if (handle.isInitialized())
        {
            bus.unsubscribe<system::event::ShaderCompiled>(handle);
        }
    }
}

void Pipeline::submitTask(device::StarDevice &device, const Handle &handle, job::TaskManager &taskSystem,
                          system::EventBus &eventBus, PipelineRecord *storedRecord)
{
    m_subscriberShaderBuildInfo.push_back(Handle());

    eventBus.subscribe<system::event::ShaderCompiled>(
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
             assert(handle.getID() <= this->m_subscriberShaderBuildInfo.size() && "Handle unknown");
             return &this->m_subscriberShaderBuildInfo[handle.getID()];
         },
         [this](const Handle &noLongerNeededHandle) {
             size_t freedNum = 0;
             for (size_t i = 0; i < this->m_subscriberShaderBuildInfo.size(); i++)
             {
                 if (!this->m_subscriberShaderBuildInfo[i].isInitialized())
                 {
                     freedNum++;
                 }
                 if (this->m_subscriberShaderBuildInfo[i] == noLongerNeededHandle)
                 {
                     this->m_subscriberShaderBuildInfo[i] = Handle();
                     freedNum++;
                 }
             }

             if (freedNum == this->m_subscriberShaderBuildInfo.size())
             {
                 this->m_subscriberShaderBuildInfo.clear();
             }
         }});
}
} // namespace star::core::device::manager