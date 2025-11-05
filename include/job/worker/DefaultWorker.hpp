#pragma once

#include "TaskContainer.hpp"

#include "core/logging/LoggingFactory.hpp"

#include <boost/thread.hpp>
#include <vulkan/vulkan.hpp>

#include <iostream>
#include <memory>
#include <type_traits>

namespace star::job::worker
{
template <typename TTask, size_t TQueueSize> class DefaultWorker
{
  public:
    DefaultWorker(std::string workerName) : m_workerName(std::move(workerName)), m_tasks(std::make_shared<job::TaskContainer<TTask, TQueueSize>>())
    {
    }
    DefaultWorker(const DefaultWorker &) = delete;
    DefaultWorker &operator=(const DefaultWorker &) = delete;
    DefaultWorker(DefaultWorker &&other) = default;
    DefaultWorker &operator=(DefaultWorker &&) = default;
    ~DefaultWorker() = default;

    void queueTask(void *task)
    {
        TTask *typedTask = static_cast<TTask *>(task);
        m_tasks->queueTask(std::move(*typedTask));
        delete typedTask;
    }

    void setCompleteMessageCommunicationStructure(
        boost::lockfree::stack<complete_tasks::CompleteTask, boost::lockfree::capacity<128>> *completeMessages)
    {
        m_completeMessages = completeMessages;
    }

    void stop()
    {
        this->shouldRun->store(false);
        if (this->thread.joinable())
            this->thread.join();
    }

    void start()
    {
        this->shouldRun->store(true);
        this->thread = boost::thread(&DefaultWorker::threadFunction, this);
    }

  protected:
    std::string m_workerName{};
    std::shared_ptr<job::TaskContainer<TTask, TQueueSize>> m_tasks;
    std::shared_ptr<boost::atomic<bool>> shouldRun =
        std::shared_ptr<boost::atomic<bool>>(new boost::atomic<bool>(true));
    boost::lockfree::stack<complete_tasks::CompleteTask, boost::lockfree::capacity<128>> *m_completeMessages =
        nullptr;
    boost::thread thread = boost::thread();

    virtual void threadFunction()
    {
        assert(m_completeMessages != nullptr);

        logStart();

        while (this->shouldRun->load())
        {
            std::optional<TTask> task = m_tasks->getQueuedTask();

            if (task.has_value())
            {
                task.value().run();

                auto message = task.value().getCompleteMessage();
                if (message.has_value())
                {
                    m_completeMessages->push(std::move(message.value()));
                }
            }
            else
            {
                boost::this_thread::sleep_for(boost::chrono::milliseconds(2));
            }
        }

        logStop();
    }

    void writeLog(boost::log::trivial::severity_level level, std::string message)
    {
        const std::string msg = m_workerName + " - " + message;
        core::logging::log(std::move(level), msg);
    }

    void logStart()
    {
        writeLog(boost::log::trivial::info, "Beginning Work");
    }

    void logStop()
    {
        writeLog(boost::log::trivial::info, "Exiting");
    }
};
} // namespace star::job::worker