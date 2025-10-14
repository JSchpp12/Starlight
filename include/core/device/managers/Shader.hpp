#pragma once

#include "Compiler.hpp"
#include "Handle.hpp"
#include "StarShader.hpp"
#include "device/managers/TaskCreatedResourceManager.hpp"

#include <array>
#include <memory>
#include <stack>
#include <vector>

namespace star::core::device::manager
{
struct ShaderRequest
{
    ShaderRequest() = default;
    ShaderRequest(StarShader shader) : shader(std::move(shader)), compiler(std::make_unique<Compiler>())
    {
    }
    ShaderRequest(StarShader shader, std::unique_ptr<Compiler> compiler)
        : shader(std::move(shader)), compiler(std::move(compiler))
    {
    }

    StarShader shader;
    std::unique_ptr<Compiler> compiler;
};
struct ShaderRecord
{
    ShaderRecord() = default;
    ShaderRecord(ShaderRequest request) : request(std::move(request))
    {
    }
    ~ShaderRecord() = default;
    ShaderRecord(const ShaderRecord &) = delete;
    ShaderRecord &operator=(const ShaderRecord &) = delete;
    ShaderRecord(ShaderRecord &&other)
        : request(std::move(other.request)), compiledShader(std::move(other.compiledShader))
    {
    }
    ShaderRecord &operator=(ShaderRecord &&other)
    {
        if (this != &other)
        {
            request = std::move(other.request);
            compiledShader = std::move(other.compiledShader);
        }
        return *this;
    }

    bool isReady() const
    {
        return compiledShader != nullptr;
    }

    void cleanupRender(core::device::StarDevice &device)
    {
        if (compiledShader)
        {
            compiledShader.reset();
        }
    }

    ShaderRequest request;
    std::unique_ptr<std::vector<uint32_t>> compiledShader = nullptr;
};
class Shader : public TaskCreatedResourceManager<ShaderRecord, ShaderRequest, Handle_Type::shader, 50>
{
  protected:
    ShaderRecord createRecord(device::StarDevice &device, ShaderRequest &&request) const override
    {
        return ShaderRecord(std::move(request));
    }

  private:
    void submitTask(device::StarDevice &device, const Handle &handle, job::TaskManager &taskSystem,
                    system::EventBus &eventBus, ShaderRecord *storedRecord) override;
};
} // namespace star::core::device::manager