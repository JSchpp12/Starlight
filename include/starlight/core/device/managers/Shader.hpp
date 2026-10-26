#pragma once

#include "StarShader.hpp"
#include "core/ShaderMacroDefinitions.hpp"
#include "device/managers/TaskCreatedResourceManager.hpp"

#include <filesystem>
#include <star_common/Handle.hpp>
#include <star_common/HandleTypeRegistry.hpp>
#include <utility>


namespace star::core::device::manager
{
struct ShaderRequest
{
    std::filesystem::path path;
    StarShader shader;
    star::core::ShaderMacroDefinitions macroDefinitions;
};
struct ShaderRecord
{
  public:
    ShaderRequest request;

    ShaderRecord() = default;
    ShaderRecord(ShaderRequest request) : request(std::move(request))
    {
    }

    bool isReady() const
    {
        return request.shader.isValid();
    }

    void setCompiledShader(StarShader shader)
    {
        request.shader = std::move(shader);
    }

    void cleanupRender(core::device::StarDevice &device)
    {
        request.shader.cleanupRender(device.getVulkanDevice());
    }
};
class Shader : public TaskCreatedResourceManager<ShaderRecord, ShaderRequest, 50>
{
  public:
    Shader()
        : TaskCreatedResourceManager<ShaderRecord, ShaderRequest, 50>(star::common::special_types::ShaderTypeName,
                                                                      "shader_event_callback")
    {
    }

  protected:
    ShaderRecord createRecord(ShaderRequest &&request) const override
    {
        return ShaderRecord(std::move(request));
    }

  private:
    void submitTask(device::StarDevice &device, const Handle &handle, job::TaskManager &taskSystem,
                    common::EventBus &eventBus, ShaderRecord *storedRecord) override;
};
} // namespace star::core::device::manager