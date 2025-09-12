#include "core/device/managers/Pipeline.hpp"

#include "core/device/system/ShaderCompiledEvent.hpp"

namespace star::core::device::manager
{
void Pipeline::submitTask(const Handle &handle, job::TaskManager &taskSystem, system::EventBus &eventBus,
                          PipelineRecord *storedRecord)
{
    eventBus.subscribe<system::ShaderCompiledEvent>([this, handle](const system::Event &e, bool &keepAlive) {
        const auto &event = static_cast<const system::ShaderCompiledEvent &>(e);

        auto *record = this->get(handle);

        for (const auto &shader : record->request.pipeline.getShaders())
        {
            if (shader == event.shaderHandle)
            {
                record->numCompiled++;
            }
        }

        if (record->numCompiled == record->request.pipeline.getShaders().size()){
            //all are compiled
            
        }

        keepAlive = false;
    });
}
} // namespace star::core::device::manager