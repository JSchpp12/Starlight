#include "FrameScheduler.hpp"

void star::job::FrameScheduler::scheduleTasksForFrame(const uint64_t &frameIndex, tasks::Task<> &&task)
{
    m_scheduledTasks[frameIndex].emplace_back(std::move(task)); 
}

std::vector<star::job::tasks::Task<>> star::job::FrameScheduler::fetchTasksForFrame(const uint64_t &frameIndex)
{
    auto it = m_scheduledTasks.find(frameIndex); 
    if (it != m_scheduledTasks.end()){
        std::vector<star::job::tasks::Task<>> frameTasks = std::move(it->second); 
        m_scheduledTasks.erase(frameIndex); 
        return frameTasks; 
    }

    return std::vector<tasks::Task<>>();
}
