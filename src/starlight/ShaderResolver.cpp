#include "starlight/ShaderResolver.hpp"

#include "starlight/command/shader/CreateShader.hpp"
#include "starlight/core/Exceptions.hpp"

#include <cassert>
#include <sstream>

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
    for (const auto &[stage, path] : m_paths)
    {
        command::shader::CreateShader cmd;
        cmd.setStage(stage).setPath(std::filesystem::path(path));
        m_bus.submit(cmd);
        const auto reply = cmd.getReply().get();
        if (!reply)
        {
            std::ostringstream oss;
            oss << "Failed to create shader from path '" << path << "':\n" << reply.error().toString();
            STAR_THROW(oss.str());
        }
        handles[stage] = reply.value();
    }

    return ShaderResolver(std::move(handles));
}
} // namespace star
