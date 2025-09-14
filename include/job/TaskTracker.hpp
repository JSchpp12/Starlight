// #pragma once

// #include "complete_tasks/CompleteTask.hpp"

// #include <boost/lockfree/spsc_queue.hpp>

// #include <memory>
// #include <mutex>
// #include <set>


// namespace star::job
// {
// class TaskTracker
// {
//   public:
//     void markTaskAsComplete(const uint16_t &taskID, complete_tasks::CompleteTask<> task)
//     {

//         while (!m_returnedMessages.push(std::move(task)))
//         {
//         }

//         markTaskAsComplete(taskID);
//     }

//     void markTaskAsComplete(const uint16_t &taskID)
//     {
//         std::lock_guard<std::mutex> lock(m_mtx);
//         assert(m_dispatchedTasks.contains(taskID) && "Task was not registered as dispatched");
//         m_dispatchedTasks.erase(taskID);
//     }

//     uint16_t getUnusedTaskID()
//     {
//         std::lock_guard<std::mutex> lock(m_mtx);

//         uint16_t id = 0;
//         while (m_dispatchedTasks.contains(id))
//         {
//             id = m_taskCounter.getNext();
//         }

//         m_dispatchedTasks.insert(id);
//     }

//   private:
//     class TaskIDCounter
//     {
//       public:
//         uint16_t getNext()
//         {
//             // uint16_t wrap around by default!
//             return m_nextID++;
//         }

//       private:
//         uint16_t m_nextID = 0;
//     };

//     boost::lockfree::spsc_queue<complete_tasks::CompleteTask<>, boost::lockfree::capacity<128>> m_returnedMessages;
//     std::set<uint16_t> m_dispatchedTasks;
//     TaskIDCounter m_taskCounter;
//     std::mutex m_mtx;
// };
// } // namespace star::job