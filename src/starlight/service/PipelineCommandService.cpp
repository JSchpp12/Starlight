#include "starlight/service/PipelineCommandService.hpp"

#include "starlight/command/pipeline/CreatePipeline.hpp"
#include "starlight/core/Exceptions.hpp"
#include "starlight/core/device/managers/Pipeline.hpp"
#include "starlight/core/device/managers/Shader.hpp"
#include "starlight/service/IOService.hpp"
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
    : m_listenForCreatePipeline(*this), m_graphicsManagers(other.m_graphicsManagers), m_cmdBus(other.m_cmdBus),
      m_ioService(other.m_ioService)
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
        m_ioService = other.m_ioService;

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
    // May be null when a custom service list wires PipelineCommandService
    // without (or before) an IOService: onCreatePipeline then passes request
    // paths through unresolved rather than dereferencing a null service. In
    // the default factory (EngineServicesFactory::createDefault) IOService is
    // parameterized first and this is never null.
    m_ioService = params.ioService;
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
        // Resolve each raw shader request path through the IOService before
        // submission so callers may pass media-relative candidates
        // (absolute/existing paths resolve to themselves via the as-is probe).
        // Without an IOService the candidate is passed through unresolved --
        // matching the pre-resolution behavior -- rather than crashing on a
        // null service.
        std::filesystem::path resolvedPath = request.path;
        if (m_ioService != nullptr)
        {
            auto resolved = m_ioService->resolve(request.path, star::io::PathKind::file);
            if (!resolved)
            {
                // A pipeline submitted over a null shader handle can never
                // finish building (it waits for a ShaderCompiled event that
                // will never fire), so fail the request outright instead of
                // handing the caller a handle that implies success.
                star::core::logging::error("Failed to resolve shader path: ", resolved.error().toString());
                cmd.getReply().set(std::unexpected(resolved.error()));
                return;
            }
            resolvedPath = std::move(resolved.value());
        }
        else
        {
            star::core::logging::warning("IOService is not available; passing shader path through unresolved: ",
                                         request.path.string());
        }

        core::device::manager::ShaderRequest managerRequest{StarShader{resolvedPath.string(), request.stage},
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
