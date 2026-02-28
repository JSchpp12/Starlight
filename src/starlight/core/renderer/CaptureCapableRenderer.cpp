// #include "core/renderer/CaptureCapableRenderer.hpp"

// #include "job/tasks/TaskFactory.hpp"
// #include "job/worker/DefaultWorker.hpp"
// #include "logging/LoggingFactory.hpp"

// namespace star::core::renderer
// {
// void CaptureCapableRenderer::prepRender(core::device::DeviceContext &context, const uint8_t &numFramesInFlight)
// {
//     DefaultRenderer::prepRender(context, numFramesInFlight);

//     if (!context.getTaskManager().isThereWorkerForTask(typeid(job::tasks::write_image_to_disk::WriteImageTask)))
//     {
//         logging::log(boost::log::trivial::info, "No worker found for image writes. Creating a new one.");
//         createWorker(context);
//     }
// }

// void CaptureCapableRenderer::cleanupRender(core::device::DeviceContext &context)
// {
//     DefaultRenderer::cleanupRender(context);
// }

// void CaptureCapableRenderer::createWorker(core::device::DeviceContext &context)
// {
//     auto worker = job::worker::DefaultWorker<job::tasks::write_image_to_disk::WriteImageTask, 256>("Image Writer");
//     auto workerWrapper = job::worker::Worker(std::move(worker));
//     context.getTaskManager().registerWorker(typeid(job::tasks::write_image_to_disk::WriteImageTask),
//                                             std::move(workerWrapper));
// }

// void CaptureCapableRenderer::frameUpdate(core::device::DeviceContext &context, const uint8_t &numFramesInFlight)
// {
//     DefaultRenderer::frameUpdate(context, numFramesInFlight);
// }
// } // namespace star::core::renderer