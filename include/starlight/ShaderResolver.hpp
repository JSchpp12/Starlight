#pragma once

#include <Enums.hpp>
#include <absl/container/flat_hash_map.h>
#include <star_common/Handle.hpp>
#include <starlight/core/CommandBus.hpp>

#include <string>

namespace star
{
class ShaderResolver
{
  public:
    class Builder
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

    ShaderResolver() = default;

    Handle resolve(Shader_Stage stage) const;

  private:
    explicit ShaderResolver(absl::flat_hash_map<Shader_Stage, Handle> handles) : m_handles(std::move(handles))
    {
    }
    absl::flat_hash_map<Shader_Stage, Handle> m_handles;
};

} // namespace star