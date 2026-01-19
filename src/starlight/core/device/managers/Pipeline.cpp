#include "core/device/managers/Pipeline.hpp"

#include "core/device/system/event/ShaderCompiled.hpp"

#include <star_common/HandleTypeRegistry.hpp>

#include <cassert>

namespace star::core::device::manager
{

void Pipeline::init(device::StarDevice *device, common::EventBus &eventBus, job::TaskManager &taskSystem)
{
    TaskCreatedResourceManager<PipelineRecord, PipelineRequest, 50>::init(device, eventBus, taskSystem);
}

void Pipeline::cleanupRender()
{
    this->TaskCreatedResourceManager<PipelineRecord, PipelineRequest, 50>::cleanupRender();
    
    std::vector<const Handle *> unsubscribers; 
    for (const auto &subscriberInfo : m_subscriberShaderBuildInfo)
    {
        if (subscriberInfo.second.isInitialized())
        {
            unsubscribers.push_back(&subscriberInfo.second);
        }
    }

    for (size_t i{0}; i < unsubscribers.size(); i++)
    {
        this->m_eventBus->unsubscribe(*unsubscribers[i]);
    }
}

void Pipeline::submitTask(device::StarDevice &device, const Handle &handle, job::TaskManager &taskSystem,
                          common::EventBus &eventBus, PipelineRecord *storedRecord)
{
    (void)storedRecord; 
    
    uint16_t key = static_cast<uint16_t>(m_subscriberShaderBuildInfo.size());
    m_subscriberShaderBuildInfo.insert(std::make_pair(key, Handle()));

    if (!common::HandleTypeRegistry::instance().contains(core::device::system::event::GetShaderCompiledEventTypeName))
    {
        common::HandleTypeRegistry::instance().registerType(
            core::device::system::event::GetShaderCompiledEventTypeName);
    }

    const uint16_t type = common::HandleTypeRegistry::instance().getTypeGuaranteedExist(
        core::device::system::event::GetShaderCompiledEventTypeName);
    eventBus.subscribe(type, {[this, handle](const star::common::IEvent &e, bool &keepAlive) {
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
                              [this, key]() -> Handle * {
                                  assert(this->m_subscriberShaderBuildInfo.contains(key));
                                  return &this->m_subscriberShaderBuildInfo.at(key);
                              },
                              [this, key](const Handle &noLongerNeededHandle) {
                                  const auto numRemoved = this->m_subscriberShaderBuildInfo.erase(key);
                              }});
}
} // namespace star::core::device::manager