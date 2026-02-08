#pragma once

#include "starlight/command/command_order/DeclareDependency.hpp"
#include "starlight/command/command_order/DeclarePass.hpp"
#include "starlight/command/command_order/GetPassInfo.hpp"
#include "starlight/command/command_order/TriggerPass.hpp"
#include "starlight/core/WorkerPool.hpp"
#include "starlight/event/EnginePhaseComplete.hpp"
#include "starlight/event/StartOfNextFrame.hpp"
#include "starlight/policy/command/ListenFor.hpp"
#include "starlight/policy/event/ListenFor.hpp"
#include "starlight/service/InitParameters.hpp"
#include "starlight/service/detail/command_order/EdgeDescription.hpp"

#include <star_common/Handle.hpp>
#include <star_common/HandleTypeRegistry.hpp>

#include <absl/container/flat_hash_map.h>

namespace star::service
{
namespace command_order
{
template <typename T>
using ListenForDeclareDependency =
    star::policy::command::ListenFor<T, star::command_order::DeclareDependency,
                                     star::command_order::declare_dependency::GetDeclareDependencyCommandTypeName,
                                     &T::onDeclareDependency>;
template <typename T>
using ListenForInitComplete =
    star::policy::event::ListenFor<T, star::event::EnginePhaseComplete, star::event::GetEnginePhaseCompleteInitTypeName,
                                   &T::onInitEnginePhaseComplete>;

template <typename T>
using ListenForStartOfNextFrame =
    star::policy::event::ListenFor<T, star::event::StartOfNextFrame, star::event::GetStartOfNextFrameTypeName,
                                   &T::onStartOfNextFrame>;

template <typename T>
using ListenForDeclarePass =
    star::policy::command::ListenFor<T, star::command_order::DeclarePass,
                                     star::command_order::declare_pass::GetDeclarePassCommandTypeName,
                                     &T::onDeclarePass>;

template <typename T>
using ListenForTriggerPass =
    star::policy::command::ListenFor<T, star::command_order::TriggerPass,
                                     star::command_order::trigger_pass::GetTriggerPassCommandTypeName,
                                     &T::onTriggerPass>;

template <typename T>
using ListenForGetPassInfo =
    star::policy::command::ListenFor<T, star::command_order::GetPassInfo,
                                     star::command_order::get_pass_info::GetPassInfoTypeName, &T::onGetPassInfo>;
} // namespace command_order

class CommandOrderService
{
  public:
    struct PassDescription
    {
        uint32_t queueFamilyIndex;
        std::vector<bool> wasProcessedOnLastFrame;
    };

    CommandOrderService();
    CommandOrderService(const CommandOrderService &) = delete;
    CommandOrderService &operator=(const CommandOrderService &) = delete;
    CommandOrderService(CommandOrderService &&);
    CommandOrderService &operator=(CommandOrderService &&);
    ~CommandOrderService() = default;

    void init();

    void negotiateWorkers(star::core::WorkerPool &pool, job::TaskManager &tm);

    void setInitParameters(star::service::InitParameters &params);

    void shutdown();

    void onDeclareDependency(star::command_order::DeclareDependency &cmd);

    void onInitEnginePhaseComplete(const star::event::EnginePhaseComplete &event, bool &keepAlive);

    void onDeclarePass(star::command_order::DeclarePass &cmd);

    void onStartOfNextFrame(const star::event::StartOfNextFrame &event, bool &keepAlive);

    void onTriggerPass(star::command_order::TriggerPass &cmd);

    void onGetPassInfo(star::command_order::GetPassInfo &cmd);

  private:
    enum Phase
    {
        Record,
        Compiled
    };

    absl::flat_hash_map<Handle, PassDescription, star::HandleHash> m_passes;
    absl::flat_hash_map<Handle, std::vector<command_order::EdgeDescription>, star::HandleHash> m_edges;
    // commands
    command_order::ListenForDeclareDependency<CommandOrderService> m_listenForDeclareDependency;
    command_order::ListenForDeclarePass<CommandOrderService> m_listenForDeclarePass;
    command_order::ListenForTriggerPass<CommandOrderService> m_listenForTriggerPass;
    command_order::ListenForGetPassInfo<CommandOrderService> m_listenForGetPassInfo;
    // events
    command_order::ListenForInitComplete<CommandOrderService> m_listenForInitPhaseComplete;
    command_order::ListenForStartOfNextFrame<CommandOrderService> m_listenForStartOfNextFrame;
    Phase m_currentPhase = Phase::Record;
    std::vector<Handle> m_triggeredPasses;
    std::vector<Handle> m_notTriggeredPasses;

    uint8_t m_lastFrameInFlightIndex = 0;
    star::core::CommandBus *m_cmdBus = nullptr;
    common::EventBus *m_evtBus = nullptr;
    star::common::FrameTracker *m_ft = nullptr;

    void initListeners(core::CommandBus &cmdBus);

    void initListeners(star::common::EventBus &evtBus);

    void cleanupListeners(core::CommandBus &cmdBus);

    void cleanupListeners(star::common::EventBus &evtBus);

    void compileOrder();

    void removeElementFromNotTriggeredPasses(const Handle &handle);

    void addEdgeRecord(const Handle &producer, const Handle &consumer);
};
} // namespace star::service