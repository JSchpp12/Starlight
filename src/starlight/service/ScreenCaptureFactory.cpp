#include "service/ScreenCaptureFactory.hpp"

#include "service/detail/screen_capture/CopyDirectorPolicy.hpp"
#include "service/detail/screen_capture/CreateDependenciesPolicies.hpp"
#include "service/detail/screen_capture/WorkerControllerPolicies.hpp"
#include "worker/detail/default_worker/BusyWaitTaskHandlingPolicy.hpp"
#include "job/worker/DefaultWorker.hpp"
#include "logging/LoggingFactory.hpp"
#include "job/tasks/WriteImageToDisk.hpp"

#include <ostream>

namespace star::service::screen_capture
{
Builder &Builder::setNumWorkers(const uint8_t &numWorkers)
{
    m_numWorkers = numWorkers;
    return *this;
}

Service Builder::build()
{
    assert(m_numWorkers > 0 && "Must have at least one worker");

    auto newWorkers = registerWorkers();

    return Service{ScreenCapture{detail::screen_capture::WorkerControllerPolicy{std::move(newWorkers)},
                                 detail::screen_capture::DefaultCreatePolicy{},
                                 detail::screen_capture::DefaultCopyPolicy{}}};
}

std::vector<job::worker::Worker::WorkerConcept *> Builder::registerWorkers()
{
    auto newWorkers = std::vector<job::worker::Worker::WorkerConcept *>(m_numWorkers);
    for (size_t i{0}; i < static_cast<size_t>(m_numWorkers); i++)
    {
        std::ostringstream oss;
        oss << "Image Writer_" << std::to_string(i);

        auto worker = m_taskManager.registerWorker(
            {job::worker::DefaultWorker{job::worker::default_worker::BusyWaitTaskHandlingPolicy<
                                            job::tasks::write_image_to_disk::WriteImageTask, 500>{true},
                                        oss.str()}},
            job::tasks::write_image_to_disk::WriteImageTypeName);

        auto *newWorker = m_taskManager.getWorker(worker);
        assert(newWorker != nullptr);

        newWorkers[i] = newWorker->getRawConcept();
    }

    return newWorkers;
}
} // namespace star::service::screen_capture