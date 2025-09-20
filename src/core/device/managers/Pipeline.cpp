#include "core/device/managers/Pipeline.hpp"

#include "core/device/system/ShaderCompiledEvent.hpp"

namespace star::core::device::manager
{
void Pipeline::submitTask(const Handle &handle, device::StarDevice &device, job::TaskManager &taskSystem,
                          system::EventBus &eventBus, PipelineRecord *storedRecord)
{
    eventBus.subscribe<system::ShaderCompiledEvent>([this, handle](const system::Event &e, bool &keepAlive) {
        const auto &event = static_cast<const system::ShaderCompiledEvent &>(e);

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