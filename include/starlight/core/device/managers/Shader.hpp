#pragma once

#include "StarShader.hpp"
#include "core/ShaderMacroDefinitions.hpp"
#include "device/managers/TaskCreatedResourceManager.hpp"
#include "job/tasks/CompileShaderResult.hpp"
#include "starlight/util/io/PathResolver.hpp"

#include <chrono>
#include <filesystem>
#include <future>
#include <optional>
#include <star_common/Handle.hpp>
#include <star_common/HandleTypeRegistry.hpp>
#include <utility>

namespace star::core::device::manager
{
/// @brief Record-level holder for a shader compile in flight. The worker publishes the result; the record resolves it
/// on the main thread.
struct ShaderCompileFuture
{
    std::future<job::tasks::compile_shader::CompileResult> future;

    [[nodiscard]] bool isReady() const
    {
        return future.valid() && future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }

    void wait() const
    {
        if (future.valid())
            future.wait();
    }

    [[nodiscard]] job::tasks::compile_shader::CompileResult take() noexcept;
};

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
    ShaderRecord() = default;
    ShaderRecord(ShaderRequest request) : request(std::move(request))
    {
    }

    /// @brief True once the compiled shader has been produced (resolved or ready to resolve).
    [[nodiscard]] bool isReady() const;

    /// @brief Consume a ready compile result into request.shader. Never blocks; no-op while pending.
    void resolve() const;

    /// @brief Block until the compile worker publishes, then resolve.
    void waitForReady() const;

    void cleanupRender(core::device::StarDevice &device);

    mutable ShaderRequest request;
    mutable std::optional<ShaderCompileFuture> compiled;
};
constexpr std::string_view GetShaderEventName = "stShdE";
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