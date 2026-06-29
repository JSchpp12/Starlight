#include "starlight/ShaderResolver.hpp"

#include "starlight/command/shader/LoadShader.hpp"

#include <cassert>

namespace star
{
Handle ShaderResolver::resolve(Shader_Stage stage) const
{
    assert(m_handles.contains(stage) && "ShaderResolver does not hold a handle for the requested stage");
    return m_handles.at(stage);
}

ShaderResolver ShaderResolver::Builder::build()
{
    absl::flat_hash_map<Shader_Stage, Handle> handles;
    for (auto &[stage, path] : m_paths)
    {
        command::shader::LoadShader cmd;
        cmd.setPath(path).setStage(stage);
        m_bus.submit(cmd);
        handles[stage] = cmd.getReply().get();
    }
    return ShaderResolver(std::move(handles));
}
} // namespace star