#include "Worker.hpp"

void star::job::workers::Worker::threadFunction()
{
    assert(m_completeMessages != nullptr);

    std::cout << "Beginning work" << std::endl;

    while (this->shouldRun.load())
    {
        tasks::Task<> task;
        if (this->taskQueue.pop(task))
        {
            task.run();

            auto message = task.getCompleteMessage();
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
