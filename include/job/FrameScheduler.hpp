#pragma once

#include "Worker.hpp"

#include <vulkan/vulkan.hpp>

namespace star::job{
    class FrameScheduler{
        public:
        void scheduleTasksForFrame(const uint64_t &frameIndex, tasks::Task<>&& task); 
        std::vector<tasks::Task<>> fetchTasksForFrame(const uint64_t &frameIndex);  
        
        private:
        std::unordered_map<uint64_t, std::vector<tasks::Task<>>> m_scheduledTasks; 

    };
}