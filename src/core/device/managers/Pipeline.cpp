#include "core/device/managers/Pipeline.hpp"

#include "core/device/system/event/ShaderCompiled.hpp"

namespace star::core::device::manager
{
void Pipeline::submitTask(device::StarDevice &device, const Handle &handle, job::TaskManager &taskSystem,
                          system::EventBus &eventBus, PipelineRecord *storedRecord)
{
    eventBus.subscribe<system::event::ShaderCompiled>([this, handle](const system::EventBase &e, bool &keepAlive) {
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
    });
}
} // namespace star::core::device::manager