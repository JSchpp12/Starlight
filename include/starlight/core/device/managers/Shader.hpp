#pragma once

#include "StarShader.hpp"
#include "core/ShaderMacroDefinitions.hpp"
#include "device/managers/TaskCreatedResourceManager.hpp"
#include "starlight/util/io/PathResolver.hpp"

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
    star::io::PathResolver pathResolver;
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
constexpr std::string_view GetShaderEventName = "stShEvt";
/// @brief Compile-time name of the shader resource type, used for handle registration and failure logging.
struct ShaderResourceTypeName
{
    static constexpr std::string_view GetUniqueTypeName()
    {
        return star::common::special_types::ShaderTypeName;
    }
};

class Shader : public TaskCreatedResourceManager<ShaderRecord, ShaderRequest, 50, ShaderResourceTypeName>
{
  public:
    Shader() : TaskCreatedResourceManager<ShaderRecord, ShaderRequest, 50, ShaderResourceTypeName>(GetShaderEventName)
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