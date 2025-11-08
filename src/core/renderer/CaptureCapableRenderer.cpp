#include "core/renderer/CaptureCapableRenderer.hpp"

#include "core/command_buffer/ScreenCapture.hpp"
#include "job/tasks/TaskFactory.hpp"
#include "job/worker/DefaultWorker.hpp"
#include "logging/LoggingFactory.hpp"

namespace star::core::renderer
{
void CaptureCapableRenderer::prepRender(core::device::DeviceContext &context, const uint8_t &numFramesInFlight)
{
    Renderer::prepRender(context, numFramesInFlight);

    if (!context.getTaskManager().isThereWorkerForTask(typeid(job::tasks::write_image_to_disk::WriteImageTask)))
    {
        logging::log(boost::log::trivial::info, "No worker found for image writes. Creating a new one.");
        createWorker(context);
    }
    
    m_screenCaptureCommands = createScreenCaptureCommands(context, numFramesInFlight);
}

void CaptureCapableRenderer::cleanupRender(core::device::DeviceContext &context){
    m_screenCaptureCommands.cleanupRender(context);

    Renderer::cleanupRender(context);
}

core::command_buffer::ScreenCapture CaptureCapableRenderer::createScreenCaptureCommands(core::device::DeviceContext &context, const uint8_t &numFramesInFlight){
    assert(this->renderToImages.size() > 0 && "Rendering target images must be created first"); 

    auto m_capture = core::command_buffer::ScreenCapture{this->renderToImages};
    m_capture.prepRender(context, numFramesInFlight, context.getRenderingSurface().getResolution());

    return m_capture;
}

void CaptureCapableRenderer::createWorker(core::device::DeviceContext &context)
{
    auto worker = job::worker::DefaultWorker<job::tasks::write_image_to_disk::WriteImageTask, 256>("Image Writer");
    auto workerWrapper = job::worker::Worker(std::move(worker));
    context.getTaskManager().registerWorker(typeid(job::tasks::write_image_to_disk::WriteImageTask),
                                            std::move(workerWrapper));
}

void CaptureCapableRenderer::frameUpdate(core::device::DeviceContext &context, const uint8_t &numFramesInFlight)
{
    Renderer::frameUpdate(context, numFramesInFlight);
}
} // namespace star::core::renderer