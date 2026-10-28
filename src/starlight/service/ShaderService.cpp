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
      m_pathResolver(std::move(other.m_pathResolver))
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
        m_pathResolver = std::move(other.m_pathResolver);

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
    assert(params.ioService != nullptr &&
           "IO service not available. The IO service must be created before the ShaderService");

    m_cmdBus = &params.commandBus;
    m_graphicsManagers = &params.graphicsManagers;
    m_pathResolver = &params.ioService->resolver();
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

    // The candidate is handed over unresolved. CompileShader::Execute resolves
    // it on the worker thread with the injected resolver; a failure is reported
    // through the compile-complete path (with the attempted locations) rather
    // than synchronously through this reply.
    core::device::manager::ShaderRequest managerRequest{request.path, StarShader{request.stage},
                                                        std::move(cmd.getMacroDefinitions()), *m_pathResolver};
    cmd.getReply().set(m_graphicsManagers->shaderManager->submit(std::move(managerRequest)));
}
} // namespace star::service
