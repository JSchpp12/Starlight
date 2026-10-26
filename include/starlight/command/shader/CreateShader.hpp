#pragma once

#include "starlight/command/shader/ShaderBuildRequest.hpp"
#include "starlight/core/ShaderMacroDefinitions.hpp"
#include "starlight/util/io/IOError.hpp"

#include <star_common/Handle.hpp>
#include <star_common/IServiceCommandWithReply.hpp>

#include <expected>
#include <filesystem>
#include <string_view>
#include <utility>

namespace star::command::shader
{
namespace create_shader
{
inline constexpr const char *GetUniqueTypeName()
{
    return "stCS";
}
} // namespace create_shader

/// @brief Request that a single shader be loaded and compiled asynchronously. The reply holds a pending handle; it does
/// not wait for compilation to finish.
struct CreateShader : public star::common::IServiceCommandWithReply<std::expected<Handle, star::io::IOError>>
{
    static inline constexpr std::string_view GetUniqueTypeName()
    {
        return create_shader::GetUniqueTypeName();
    }

    CreateShader &setStage(Shader_Stage stage)
    {
        m_request.stage = stage;
        return *this;
    }

    CreateShader &setPath(std::filesystem::path path)
    {
        m_request.path = std::move(path);
        return *this;
    }

    CreateShader &setMacroDefinitions(const star::core::ShaderMacroDefinitions &macroDefinitions)
    {
        m_request.macroDefinitions = macroDefinitions;
        return *this;
    }

    CreateShader &setMacroDefinitions(star::core::ShaderMacroDefinitions &&macroDefinitions)
    {
        m_request.macroDefinitions = std::move(macroDefinitions);
        return *this;
    }

    Shader_Stage getStage() const
    {
        return m_request.stage;
    }

    const std::filesystem::path &getPath() const
    {
        return m_request.path;
    }

    const ShaderBuildRequest &getBuildRequest() const
    {
        return m_request;
    }

    const star::core::ShaderMacroDefinitions &getMacroDefinitions() const
    {
        return m_request.macroDefinitions;
    }
    star::core::ShaderMacroDefinitions &getMacroDefinitions()
    {
        return m_request.macroDefinitions;
    }

  private:
    ShaderBuildRequest m_request;
};
} // namespace star::command::shader
