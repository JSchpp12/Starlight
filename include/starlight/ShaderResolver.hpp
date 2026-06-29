#pragma once

#include <absl/container/flat_hash_map.h>
#include <Enums.hpp>
#include <star_common/Handle.hpp>
#include <starlight/core/CommandBus.hpp>

#include <string>

namespace star
{
class ShaderResolver
{
  public:
    class Builder;
    ShaderResolver() = default;

    Handle resolve(Shader_Stage stage) const;
  private:
    explicit ShaderResolver(absl::flat_hash_map<Shader_Stage, Handle> handles) : m_handles(std::move(handles)) {}
    absl::flat_hash_map<Shader_Stage, Handle> m_handles;
};

class ShaderResolver::Builder
{
  public:
    explicit Builder(core::CommandBus &bus) : m_bus(bus)
    {
    }

    Builder &setShader(Shader_Stage stage, std::string path)
    {
        m_paths[stage] = std::move(path);
        return *this;
    }

    ShaderResolver build();

  private:
    core::CommandBus &m_bus;
    absl::flat_hash_map<Shader_Stage, std::string> m_paths;
};
} // namespace star