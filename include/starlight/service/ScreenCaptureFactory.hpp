#pragma once

#include "ScreenCapture.hpp"
#include "Service.hpp"
#include "job/tasks/WriteImageToDisk.hpp"

#include "job/worker/DefaultWorker.hpp"
#include "job/worker/Worker.hpp"
#include "logging/LoggingFactory.hpp"

namespace star::service::screen_capture
{
class Builder
{
  public:
    Builder(core::device::StarDevice &device, job::TaskManager &taskManager)
        : m_device(device), m_taskManager(taskManager)
    {
    }

    Builder &setNumWorkers(const uint8_t &numWorkers); 
    
    Service build();
  private:
    core::device::StarDevice &m_device;
    job::TaskManager &m_taskManager;
    uint8_t m_numWorkers = 1;

    std::vector<job::worker::Worker::WorkerConcept *> registerWorkers();
};
} // namespace star::service::screen_capture