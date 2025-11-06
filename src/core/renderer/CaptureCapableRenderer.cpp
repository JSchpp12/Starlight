#include "core/renderer/CaptureCapableRenderer.hpp"

#include "core/command_buffer/ScreenCapture.hpp"
#include "job/tasks/TaskFactory.hpp"
#include "job/worker/DefaultWorker.hpp"

namespace star::core::renderer
{
void CaptureCapableRenderer::prepRender(core::device::DeviceContext &context, const uint8_t &numFramesInFlight)
{
    // if (context.getTaskManager().isThereWorkerForTask(typeid(job::tasks::task_factory::write_image_to_disk::WritePayload))){
        
    // }
    createWorker(context);   
}

void CaptureCapableRenderer::createWorker(core::device::DeviceContext &context){
    // context.getTaskManager()
}
} // namespace star::core::renderer