#pragma once

#include <star_common/IServiceCommandWithReply.hpp>
#include <star_common/Handle.hpp>
#include <Enums.hpp>

#include <string>
#include <string_view>

namespace star::command::shader
{
namespace load_shader
{
inline constexpr const char *GetUniqueTypeName()
{
    return "scLs";
};
} // namespace load_shader

struct LoadShader : public star::common::IServiceCommandWithReply<Handle>
{
    static inline constexpr std::string_view GetUniqueTypeName()
    {
        return load_shader::GetUniqueTypeName();
    }

    LoadShader &setPath(std::string path)
    {
        m_path = std::move(path);
        return *this;
    }

    LoadShader &setStage(Shader_Stage stage)
    {
        m_stage = stage;
        return *this;
    }

    const std::string &getPath() const
    {
        return m_path;
    }

    Shader_Stage getStage() const
    {
        return m_stage;
    }

  private:
    std::string m_path;
    Shader_Stage m_stage = Shader_Stage::none;
};
} // namespace star::command::shader