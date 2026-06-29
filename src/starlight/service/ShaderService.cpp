#include "starlight/service/ShaderService.hpp"

#include "starlight/core/device/managers/Shader.hpp"
#include "starlight/virtual/StarShader.hpp"

namespace star::service
{
ShaderService::ShaderService() : m_listenForLoadShader(*this)
{
}

ShaderService::ShaderService(ShaderService &&other)
    : m_listenForLoadShader(*this), m_graphicsManagers(other.m_graphicsManagers), m_cmdBus(other.m_cmdBus)
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
    m_listenForLoadShader.init(bus);
}

void ShaderService::cleanupListeners(core::CommandBus &bus)
{
    m_listenForLoadShader.cleanup(bus);
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
}

void ShaderService::onLoadShader(star::command::shader::LoadShader &cmd)
{
    assert(m_graphicsManagers != nullptr && m_graphicsManagers->shaderManager != nullptr &&
           "Shader manager not available");

    StarShader shader{cmd.getPath(), cmd.getStage()};
    Handle handle = m_graphicsManagers->shaderManager->submit(std::move(shader));
    cmd.getReply().set(handle);
}
} // namespace star::service