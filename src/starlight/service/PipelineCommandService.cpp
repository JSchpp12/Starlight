#include "starlight/service/PipelineCommandService.hpp"

#include "starlight/command/pipeline/CreatePipeline.hpp"
#include "starlight/core/Exceptions.hpp"
#include "starlight/core/device/managers/Pipeline.hpp"
#include "starlight/core/device/managers/Shader.hpp"
#include "starlight/virtual/StarPipeline.hpp"
#include "starlight/virtual/StarShader.hpp"

#include <utility>
#include <vector>

namespace star::service
{
PipelineCommandService::PipelineCommandService() : m_listenForCreatePipeline(*this)
{
}

PipelineCommandService::PipelineCommandService(PipelineCommandService &&other)
    : m_listenForCreatePipeline(*this), m_graphicsManagers(other.m_graphicsManagers), m_cmdBus(other.m_cmdBus)
{
    if (m_cmdBus != nullptr)
    {
        other.cleanupListeners(*m_cmdBus);
        initListeners(*m_cmdBus);
    }
}

PipelineCommandService &PipelineCommandService::operator=(PipelineCommandService &&other)
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

void PipelineCommandService::initListeners(core::CommandBus &bus)
{
    m_listenForCreatePipeline.init(bus);
}

void PipelineCommandService::cleanupListeners(core::CommandBus &bus)
{
    m_listenForCreatePipeline.cleanup(bus);
}

void PipelineCommandService::init()
{
    assert(m_cmdBus != nullptr && "Command bus not saved from initParameters");

    initListeners(*m_cmdBus);
}

void PipelineCommandService::shutdown()
{
    assert(m_cmdBus != nullptr);

    cleanupListeners(*m_cmdBus);
}

void PipelineCommandService::setInitParameters(InitParameters &params)
{
    m_cmdBus = &params.commandBus;
    m_graphicsManagers = &params.graphicsManagers;
}

void PipelineCommandService::onCreatePipeline(star::command::pipeline::CreatePipeline &cmd)
{
    assert(m_graphicsManagers != nullptr && m_graphicsManagers->shaderManager != nullptr &&
           m_graphicsManagers->pipelineManager != nullptr && "Pipeline managers not available");

    cmd.validate();

    // Resolve any raw shader requests first so the pipeline manager can subscribe
    // to their ShaderCompiled events exactly like the direct-submission path did.
    std::vector<Handle> shaderHandles;
    shaderHandles.reserve(cmd.getShaderCount());

    for (const auto &handle : cmd.getShaderHandles())
    {
        shaderHandles.push_back(handle);
    }

    for (const auto &request : cmd.getShaderRequests())
    {
        core::device::manager::ShaderRequest managerRequest{StarShader{request.pathString(), request.stage},
                                                            request.compiler};
        shaderHandles.push_back(m_graphicsManagers->shaderManager->submit(std::move(managerRequest)));
    }

    PipelineProvider provider = [&]() {
        if (cmd.getPipelineType() == PipelineType::Compute)
        {
            return PipelineProvider(shaderHandles.front(), cmd.getPipelineLayout());
        }

        if (cmd.getGraphicsOverrides().has_value())
        {
            return PipelineProvider(shaderHandles, cmd.getPipelineLayout(), *cmd.getGraphicsOverrides());
        }

        return PipelineProvider(shaderHandles, cmd.getPipelineLayout());
    }();

    core::device::manager::PipelineRequest request{std::move(provider), cmd.getSwapChainExtent(),
                                                   cmd.getRenderingTargetInfo()};

    cmd.getReply().set(m_graphicsManagers->pipelineManager->submit(std::move(request)));
}
} // namespace star::service
