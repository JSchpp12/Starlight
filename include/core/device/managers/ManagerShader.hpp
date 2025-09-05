#pragma once

#include "StarShader.hpp"
#include "job/TaskManager.hpp"

#include <Handle.hpp>

#include <array>
#include <memory>
#include <stack>
#include <vector>


namespace star::core::device::manager
{
class Shader
{
  public:
    struct Record
    {
        Record() = default;
        Record(StarShader shader) : shader(std::move(shader)){}
        ~Record() = default;
        Record(const Record &) = delete;
        Record &operator=(const Record &) = delete;
        Record(Record &&other) : shader(other.shader), compiledShader(std::move(other.compiledShader)){}
        Record &operator=(Record &&other){
          if (this != &other){
            shader = other.shader;
            compiledShader = std::move(other.compiledShader); 
          }
          return *this;
        }

        StarShader shader;
        std::unique_ptr<std::vector<uint32_t>> compiledShader = nullptr;
    };

    Handle submit(job::TaskManager &taskSystem, StarShader shader);

    Record &get(const Handle &handle);

    bool isReady(const Handle &handle); 

  private:
    std::stack<size_t> m_skippedSpaces;
    std::array<Record, 50> m_shaders;
    uint32_t m_nextSpace = 0;

    void submitTask(job::TaskManager &taskSystem, Record &storedRecord, const Handle &handle);
};
} // namespace star::core::device::manager