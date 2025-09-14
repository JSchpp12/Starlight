#include "core/device/managers/Pipeline.hpp"

#include "core/device/system/ShaderCompiledEvent.hpp"
#include "job/tasks/TaskFactory.hpp"

namespace star::core::device::manager
{
void Pipeline::submitTask(const Handle &handle, device::StarDevice &device, job::TaskManager &taskSystem, system::EventBus &eventBus,
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

        keepAlive = false;
    });
}
} // namespace star::core::device::manager