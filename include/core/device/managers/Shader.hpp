#pragma once

#include "Compiler.hpp"
#include "StarShader.hpp"
#include "device/managers/TaskCreatedResourceManager.hpp"

#include <starlight/common/Handle.hpp>
#include <starlight/common/HandleTypeRegistry.hpp>

#include <array>
#include <memory>
#include <stack>
#include <vector>

namespace star::core::device::manager
{
struct ShaderRequest
{
    ShaderRequest() = default;
    ShaderRequest(StarShader shader) : shader(std::move(shader))
    {
    }
    ShaderRequest(StarShader shader, Compiler compiler)
        : shader(std::move(shader)), compiler(std::move(compiler))
    {
    }

    StarShader shader;
    Compiler compiler;
};
struct ShaderRecord
{
    ShaderRecord() = default;
    ShaderRecord(ShaderRequest request) : request(std::move(request))
    {
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
    std::shared_ptr<std::vector<uint32_t>> compiledShader = nullptr;
};
class Shader : public TaskCreatedResourceManager<ShaderRecord, ShaderRequest, 50>
{
  public:
    Shader() : TaskCreatedResourceManager<ShaderRecord, ShaderRequest, 50>(star::common::special_types::ShaderTypeName(), "shader_event_callback")
    {
    }

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