#pragma once

#include "Worker.hpp"

#include <memory>
#include <typeindex>
#include <unordered_map>

namespace star::Job
{
class Manager
{
  public:
    Manager() : m_defaultWorker(CreateDefaultWorker()){};
    ~Manager() = default;

    Manager(const Manager &) = delete;
    Manager &operator=(const Manager &) = delete;

    Manager(Manager &&) = default;
    Manager &operator=(Manager &&) = default;

    Worker &registerWorker(const std::type_index &taskType)
    {
      auto worker = std::make_shared<Worker>();
      Worker* raw = worker.get(); 
      m_workers[taskType].push_back(worker);

      return *raw;
    }

    Worker &registerWorker(const std::type_index &taskType, std::shared_ptr<Worker> newWorker){
      auto *raw = newWorker.get(); 
      m_workers[taskType].push_back(newWorker); 
      return *raw;
    }

    void startAll(){
      for (auto &[type, list] : m_workers){
        for (auto& w : list){
          w->start();
        }
      }

      m_defaultWorker->start();
    }

    void stopAll(){
      for (auto &[type, list] : m_workers){
        for (auto& w : list){
          w->stop();
        }
      }

      m_defaultWorker->stop();
    }

    Worker* getWorker(const std::type_index &taskType, const size_t &index = 0){
      auto it = m_workers.find(taskType);
      if (it == m_workers.end() || index >= it->second.size()){
        return nullptr;
      }

      return it->second[index].get();
    }

    void submitTask(Task<>&& newTask){
      m_defaultWorker->queueTask(std::move(newTask));
    }

  private:
    std::unordered_map<std::type_index, std::vector<std::shared_ptr<Worker>>> m_workers =
        std::unordered_map<std::type_index, std::vector<std::shared_ptr<Worker>>>();

    std::unique_ptr<Worker> m_defaultWorker = nullptr; 

    static std::unique_ptr<Worker > CreateDefaultWorker(){
      return std::make_unique<Worker>();
    }
};
} // namespace star::Job