#include "starlight/service/CommandOrderService.hpp"

namespace star::service
{
CommandOrderService::CommandOrderService()
    : m_passes(), m_listenForDeclareDependency(*this), m_listenForDeclarePass(*this), m_listenForTriggerPass(*this),
      m_listenForGetPassInfo(*this), m_listenForInitPhaseComplete(*this), m_listenForStartOfNextFrame(*this)
{
}

CommandOrderService::CommandOrderService(CommandOrderService &&other)
    : m_passes(std::move(other.m_passes)), m_listenForDeclareDependency(*this), m_listenForDeclarePass(*this),
      m_listenForTriggerPass(*this), m_listenForGetPassInfo(*this), m_listenForInitPhaseComplete(*this),
      m_listenForStartOfNextFrame(*this)
{
    if (m_cmdBus != nullptr && m_evtBus != nullptr)
    {
        other.cleanupListeners(*m_cmdBus);
        other.cleanupListeners(*m_evtBus);

        initListeners(*m_cmdBus);
        initListeners(*m_evtBus);
    }
}

CommandOrderService &CommandOrderService::operator=(CommandOrderService &&other)
{
    if (this != &other)
    {
        m_passes = std::move(other.m_passes);

        if (m_cmdBus != nullptr && m_evtBus != nullptr)
        {
            other.cleanupListeners(*m_cmdBus);
            other.cleanupListeners(*m_evtBus);

            initListeners(*m_cmdBus);
            initListeners(*m_evtBus);
        }
    }

    return *this;
}

void CommandOrderService::init()
{
    assert(m_cmdBus != nullptr);

    initListeners(*m_cmdBus);
    initListeners(*m_evtBus);

    m_lastFrameInFlightIndex = m_ft->getCurrent().getFrameInFlightIndex();
}

void CommandOrderService::negotiateWorkers(star::core::WorkerPool &pool, job::TaskManager &tm)
{
}

void CommandOrderService::setInitParameters(star::service::InitParameters &params)
{
    m_cmdBus = &params.commandBus;
    m_evtBus = &params.eventBus;
    m_ft = &params.flightTracker;
}

void CommandOrderService::shutdown()
{
    assert(m_cmdBus != nullptr && m_evtBus != nullptr);

    cleanupListeners(*m_cmdBus);
    cleanupListeners(*m_evtBus);
}

void CommandOrderService::onDeclareDependency(star::command_order::DeclareDependency &cmd)
{
    assert(m_passes.contains(cmd.getSrc()) && m_passes.contains(cmd.getDep()));

    addEdgeRecord(cmd.getSrc(), cmd.getDep());
}

void CommandOrderService::onDeclarePass(star::command_order::DeclarePass &cmd)
{
    const Handle &passHandle = cmd.getPassHandle();
    const uint32_t queueFamilyIndex = cmd.getQueueFamily();
    auto mem = std::vector<bool>(m_ft->getSetup().getNumFramesInFlight(), false);

    m_passes.insert(std::make_pair(passHandle, PassDescription{queueFamilyIndex, std::move(mem)}));

    m_notTriggeredPasses.emplace_back(passHandle);
}

void CommandOrderService::onInitEnginePhaseComplete(const star::event::EnginePhaseComplete &event, bool &keepAlive)
{
    keepAlive = false;

    compileOrder();
}

void CommandOrderService::onTriggerPass(star::command_order::TriggerPass &cmd)
{
    assert(m_passes.contains(cmd.passHandle) && "Pass handle is not valid");

    m_triggeredPasses.emplace_back(cmd.passHandle);

    removeElementFromNotTriggeredPasses(cmd.passHandle);
}

void CommandOrderService::onStartOfNextFrame(const event::StartOfNextFrame &event, bool &keepAlive)
{
    assert(m_ft != nullptr);

    for (size_t i{0}; i < m_notTriggeredPasses.size(); i++)
    {
        m_passes[m_notTriggeredPasses[i]].wasProcessedOnLastFrame[m_lastFrameInFlightIndex] = false;
    }

    for (auto &record : m_triggeredPasses)
    {
        m_passes[record].wasProcessedOnLastFrame[m_lastFrameInFlightIndex] = true;
        m_notTriggeredPasses.emplace_back(record); 
    }

    m_triggeredPasses.clear();
    m_lastFrameInFlightIndex = m_ft->getCurrent().getFrameInFlightIndex();
    keepAlive = true;
}

void CommandOrderService::onGetPassInfo(star::command_order::GetPassInfo &cmd)
{
    assert(m_passes.contains(cmd.pass) && "Provided pass does not exist");

    bool isTriggered = false;
    for (const auto &triggered : m_triggeredPasses)
    {
        if (triggered == cmd.pass)
        {
            isTriggered = true;
            break;
        }
    }

    const auto &info = m_passes[cmd.pass];
    cmd.getReply().set(star::command_order::get_pass_info::GatheredPassInfo(
        isTriggered, &info.queueFamilyIndex, &info.wasProcessedOnLastFrame,
        m_edges.contains(cmd.pass) ? &m_edges[cmd.pass] : nullptr));
}

void CommandOrderService::initListeners(core::CommandBus &cmdBus)
{
    m_listenForDeclareDependency.init(cmdBus);
    m_listenForDeclarePass.init(cmdBus);
    m_listenForTriggerPass.init(cmdBus);
    m_listenForGetPassInfo.init(cmdBus);
}

void CommandOrderService::initListeners(common::EventBus &evtBus)
{
    m_listenForInitPhaseComplete.init(evtBus);
    m_listenForStartOfNextFrame.init(evtBus);
}

void CommandOrderService::cleanupListeners(core::CommandBus &cmdBus)
{
    m_listenForDeclareDependency.cleanup(cmdBus);
    m_listenForDeclarePass.cleanup(cmdBus);
    m_listenForTriggerPass.cleanup(cmdBus);
    m_listenForGetPassInfo.cleanup(cmdBus);
}

void CommandOrderService::cleanupListeners(common::EventBus &evtBus)
{
    m_listenForInitPhaseComplete.cleanup(evtBus);
    m_listenForStartOfNextFrame.cleanup(evtBus);
}

void CommandOrderService::compileOrder()
{
    assert(m_currentPhase != Phase::Compiled && "Cannot be compiled more than once");

    m_currentPhase = Phase::Compiled;
}

void CommandOrderService::removeElementFromNotTriggeredPasses(const Handle &handle)
{
    size_t index = 0;
    for (size_t i{0}; i < m_notTriggeredPasses.size(); i++)
    {
        if (m_notTriggeredPasses[i] == handle)
        {
            index = i;
            break;
        }
    }
    m_notTriggeredPasses.erase(m_notTriggeredPasses.begin() + static_cast<std::ptrdiff_t>(index));
}

void CommandOrderService::addEdgeRecord(const Handle &producer, const Handle &consumer)
{
    command_order::EdgeDescription edge{.producer = producer, .consumer = consumer};
    if (m_edges.contains(producer))
    {
        m_edges[producer].emplace_back(edge);
    }
    else
    {
        m_edges.insert(std::make_pair(producer, std::vector<command_order::EdgeDescription>{edge}));
    }

    // check consumer too
    if (m_edges.contains(consumer))
    {
        m_edges[consumer].emplace_back(edge);
    }
    else
    {
        m_edges.insert(std::make_pair(consumer, std::vector<command_order::EdgeDescription>{edge}));
    }
}
} // namespace star::service