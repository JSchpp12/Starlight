#include "Worker.hpp"

void star::job::worker::Worker::threadFunction()
{
    assert(m_completeMessages != nullptr);

    std::cout << "Beginning work" << std::endl;

    while (this->shouldRun.load())
    {
        std::optional<tasks::Task<>> task = m_tasks.getQueuedTask(); 

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

    std::cout << "Exiting" << std::endl;
}
