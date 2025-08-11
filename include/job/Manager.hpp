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
    Manager() = default;
    ~Manager() = default;

    Manager(const Manager &) = delete;
    Manager &operator=(const Manager &) = delete;

    Manager(Manager &&) = default;
    Manager &operator=(Manager &&) = default;

    template <typename T, typename TTask> Worker<T, TTask> &registerWorker()
    {
      auto worker = std::make_unique<Worker<T, TTask>>();
      Worker<T, TTask>* raw = worker.get(); 
      this->workers[typeid(Worker<T, TTask>)].push_back(std::move(worker));

      return *raw;
    }

    template <typename T, typename TTask> Worker<T, TTask> &registerWorker(std::unique_ptr<Worker<T, TTask>> newWorker){
      Worker<T, TTask>* raw = newWorker.get(); 
      this->workers[typeid(Worker<T, TTask>)].push_back(std::move(newWorker)); 
      return *raw;
    }

    void startAll(){
      for (auto &[type, list] : this->workers){
        for (auto& w : list){
          w->start();
        }
      }
    }

    void stopAll(){
      for (auto &[type, list] : this->workers){
        for (auto& w : list){
          w->stop();
        }
      }
    }

    template <typename T, typename TTask> Worker<T, TTask> &getWorker(const size_t &index = 0){
      auto it = this->workers.find(typeid(Worker<T, TTask>));
      if (it == this->workers.end() || index  >= it->second.size()){
        throw std::runtime_error("Worker not found for requested type"); 
      }

      auto *raw = static_cast<Worker<T, TTask>*>(it->second[index].get());
      return *raw;
    }

  private:
    std::unordered_map<std::type_index, std::vector<std::unique_ptr<IWorkerBase>>> workers =
        std::unordered_map<std::type_index, std::vector<std::unique_ptr<IWorkerBase>>>();
};
} // namespace star::Job