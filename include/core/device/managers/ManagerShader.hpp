#pragma once

#include "Handle.hpp"
#include "StarShader.hpp"
#include "device/managers/Manager.hpp"

#include <array>
#include <memory>
#include <stack>
#include <vector>

namespace star::core::device::manager
{
struct ShaderRecord
{
    ShaderRecord() = default;
    ShaderRecord(StarShader shader) : shader(std::move(shader))
    {
    }
    ~ShaderRecord() = default;
    ShaderRecord(const ShaderRecord &) = delete;
    ShaderRecord &operator=(const ShaderRecord &) = delete;
    ShaderRecord(ShaderRecord &&other) : shader(other.shader), compiledShader(std::move(other.compiledShader))
    {
    }
    ShaderRecord &operator=(ShaderRecord &&other)
    {
        if (this != &other)
        {
            shader = other.shader;
            compiledShader = std::move(other.compiledShader);
        }
        return *this;
    }

    bool isReady() const
    {
        return compiledShader != nullptr;
    }

    StarShader shader = StarShader();
    std::unique_ptr<std::vector<uint32_t>> compiledShader = nullptr;
};
class Shader : public Manager<ShaderRecord, StarShader, 50>
{
  public:
  protected:
    Handle_Type getHandleType() const override
    {
        return Handle_Type::shader;
    }

  private:
    void submitTask(const Handle &handle, job::TaskManager &taskSystem, system::EventBus &eventBus,
                    ShaderRecord *storedRecord) override;
};
} // namespace star::core::device::manager