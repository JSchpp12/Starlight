#include "starlight/service/ShaderService.hpp"

#include "starlight/command/shader/CreateShader.hpp"
#include "starlight/core/Exceptions.hpp"
#include "starlight/core/device/managers/Shader.hpp"
#include "starlight/service/IOService.hpp"
#include "starlight/virtual/StarShader.hpp"

namespace star::service
{
ShaderService::ShaderService() : m_listenForCreateShader(*this)
{
}

ShaderService::ShaderService(ShaderService &&other)
    : m_listenForCreateShader(*this), m_graphicsManagers(other.m_graphicsManagers), m_cmdBus(other.m_cmdBus),
      m_ioService(other.m_ioService)
{
    if (m_cmdBus != nullptr)
    {
        other.cleanupListeners(*m_cmdBus);
        initListeners(*m_cmdBus);
    }
}

ShaderService &ShaderService::operator=(ShaderService &&other)
{
    if (this != &other)
    {
        m_graphicsManagers = other.m_graphicsManagers;
        m_cmdBus = other.m_cmdBus;
        m_ioService = other.m_ioService;

        if (m_cmdBus != nullptr)
        {
            other.cleanupListeners(*m_cmdBus);
            initListeners(*m_cmdBus);
        }
    }

    return *this;
}

void ShaderService::initListeners(core::CommandBus &bus)
{
    m_listenForCreateShader.init(bus);
}

void ShaderService::cleanupListeners(core::CommandBus &bus)
{
    m_listenForCreateShader.cleanup(bus);
}

void ShaderService::init()
{
    assert(m_cmdBus != nullptr && "Command bus not saved from initParameters");

    initListeners(*m_cmdBus);
}

void ShaderService::shutdown()
{
    assert(m_cmdBus != nullptr);

    cleanupListeners(*m_cmdBus);
}

void ShaderService::setInitParameters(InitParameters &params)
{
    m_cmdBus = &params.commandBus;
    m_graphicsManagers = &params.graphicsManagers;
    // May be null when a custom service list wires ShaderService without (or
    // before) an IOService: onCreateShader then passes request paths through
    // unresolved rather than dereferencing a null service. In the default
    // factory (EngineServicesFactory::createDefault) IOService is parameterized
    // first and this is never null.
    m_ioService = params.ioService;
}

void ShaderService::onCreateShader(star::command::shader::CreateShader &cmd)
{
    assert(m_graphicsManagers != nullptr && m_graphicsManagers->shaderManager != nullptr &&
           "Shader manager not available");

    const auto &request = cmd.getBuildRequest();
    if (!request.isValid())
    {
        STAR_THROW("CreateShader received an invalid shader build request");
    }

    // Resolve the shader path through the IOService before submission so
    // callers may pass media-relative candidates (absolute/existing paths
    // resolve to themselves via the as-is probe). Without an IOService the
    // candidate is passed through unresolved -- matching the pre-resolution
    // behavior -- rather than crashing on a null service.
    std::filesystem::path resolvedPath = request.path;
    if (m_ioService != nullptr)
    {
        auto resolved = m_ioService->resolve(request.path, star::io::PathKind::file);
        if (!resolved)
        {
            // Surface the failure through the reply so callers can abort
            // instead of silently holding a null handle.
            star::core::logging::error("Failed to resolve shader path: ", resolved.error().toString());
            cmd.getReply().set(std::unexpected(resolved.error()));
            return;
        }
        resolvedPath = std::move(resolved.value());
    }
    else
    {
        star::core::logging::warning("IOService is not available; passing shader path through unresolved: ",
                                     request.path);
    }

    core::device::manager::ShaderRequest managerRequest{std::move(resolvedPath), StarShader{request.stage},
                                                        std::move(cmd.getMacroDefinitions())};
    cmd.getReply().set(m_graphicsManagers->shaderManager->submit(std::move(managerRequest)));
}
} // namespace star::service
