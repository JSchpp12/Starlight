#include "service/ScreenCaptureFactory.hpp"

#include "service/detail/screen_capture/CopyPolicies.hpp"
#include "service/detail/screen_capture/CreateDependenciesPolicies.hpp"
#include "service/detail/screen_capture/WorkerControllerPolicies.hpp"

#include "worker/default_worker/detail/ThreadTaskHandlingPolicies.hpp"

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

    if (m_numWorkers > 1)
    {
        std::ostringstream oss;
        oss << "More than one worker is not currently supported";
        core::logging::log(boost::log::trivial::error, oss.str());

        throw std::runtime_error(oss.str());
    }

    auto newWorkers = registerWorkers();

    return Service{ScreenCapture{detail::screen_capture::WorkerControllerPolicy{std::move(newWorkers)},
                                 detail::screen_capture::DefaultCreatePolicy{},
                                 detail::screen_capture::DefaultCopyPolicy{}}};
}

std::vector<job::worker::Worker::WorkerConcept *> Builder::registerWorkers()
{
    auto newWorkers = std::vector<job::worker::Worker::WorkerConcept *>(m_numWorkers);
    for (uint8_t i = 0; i < m_numWorkers; i++)
    {
        std::ostringstream oss;
        oss << "Image Writer_" << std::to_string(i);

        newWorkers[i] =
            m_taskManager
                .registerWorker(
                    typeid(job::tasks::write_image_to_disk::WriteImageTask),
                    {job::worker::DefaultWorker(
                        job::worker::default_worker::DefaultThreadTaskHandlingPolicy<job::tasks::write_image_to_disk::WriteImageTask, 500>{},
                        oss.str())})
                .getRawConcept();
    }

    return newWorkers;
}
} // namespace star::service::screen_capture