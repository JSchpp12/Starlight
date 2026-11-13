#pragma once

#include "core/graphics/GPUWorkSyncInfo.hpp"
#include "starlight/command/material/MakeMaterial.hpp"
#include "starlight/command/material/SetMaterialResource.hpp"
#include "starlight/core/WorkerPool.hpp"
#include "starlight/core/transfer/ReadinessCondition.hpp"
#include "starlight/core/transfer/ReadinessFailurePolicy.hpp"
#include "starlight/event/MaterialSwapCommitted.hpp"
#include "starlight/event/PassCPURecordingWaitResolved.hpp"
#include "starlight/event/StartOfNextFrame.hpp"
#include "starlight/policy/command/ListenFor.hpp"
#include "starlight/policy/event/ListenFor.hpp"
#include "starlight/policy/event/ListenForPassCPURecordingWaitResolved.hpp"
#include "starlight/service/InitParameters.hpp"

#include <star_common/Handle.hpp>

#include <optional>
#include <vector>

namespace star::core::device
{
class DeviceContext;
}

namespace star::service
{
template <typename T>
using ListenForSetMaterialResource =
    star::policy::command::ListenFor<T, star::command::material::SetMaterialResource,
                                     star::command::material::set_material_resource::GetTypeName,
                                     &T::onSetMaterialResource>;
template <typename T>
using ListenForMakeMaterial =
    star::policy::command::ListenFor<T, star::command::material::MakeMaterial,
                                     star::command::material::make_material::GetTypeName, &T::onMakeMaterial>;
template <typename T>
using ListenForStartOfNextFrame =
    star::policy::event::ListenFor<T, star::event::StartOfNextFrame,
                                   star::event::start_of_next_frame::GetUniqueTypeName, &T::onStartOfNextFrame>;

/// @brief A material swap awaiting the owning pass's readiness result.
/// @details The material keeps its previous binding until commit; commit creates and binds the new StarShaderInfo.
struct SwapTransaction
{
    command::material::MaterialTarget target;
    Handle pass;
    Handle newResource;
    Handle previousResource;
    command::material::SwapFailureHandler onFailure{nullptr};
    std::optional<core::graphics::SemaphoreInfo> gate;
    setup::RenderResourceSetup::DefinitionId definition{setup::RenderResourceSetup::InvalidDefinitionId};
    std::vector<Handle> conditionResources;
    bool sawResult{false};
    bool pending{false};
};

/// @brief Material lifecycle service: creation, resource switching, and retirement.
/// @details A swap is recorded as a transaction and the material keeps its previous binding until the owning pass
/// confirms readiness. Commit creates and binds the new resource; failure runs the transaction's handler.
class MaterialService
{
  public:
    /// @brief Abandon the swap, retire the new resource, and notify the app. The previous binding is untouched.
    static void RollbackSwap(MaterialService &service, SwapTransaction &transaction);
    /// @brief Log and abort the process.
    static void TerminateSwap(MaterialService &service, SwapTransaction &transaction);

    explicit MaterialService(command::material::SwapFailureHandler defaultOnFailure = &MaterialService::TerminateSwap)
        : m_defaultOnFailure(defaultOnFailure), m_listenForSet(*this), m_listenForMake(*this), m_listenForFrame(*this),
          m_listenForWaitResolved(*this)
    {
    }
    MaterialService(const MaterialService &) = delete;
    MaterialService &operator=(const MaterialService &) = delete;
    MaterialService(MaterialService &&other) noexcept;
    MaterialService &operator=(MaterialService &&other) noexcept = delete;
    ~MaterialService() = default;

    void negotiateWorkers(core::WorkerPool &pool, job::TaskManager &tm)
    {
    }

    void setInitParameters(InitParameters &params);
    void init();
    void shutdown();

    void onMakeMaterial(command::material::MakeMaterial &cmd);
    void onSetMaterialResource(command::material::SetMaterialResource &cmd);
    void onStartOfNextFrame(const event::StartOfNextFrame &event, bool &keepAlive);
    void onPassCPURecordingWaitResolved(const event::PassCPURecordingWaitResolved &event, bool &keepAlive);

  private:
    struct RetireEntry
    {
        Handle handle;
        bool isImage{false};
        /// @brief Recording-submission gate captured at swap time. Empty when the pass could not report one, in
        /// which case the entry is held for the teardown backstop.
        std::optional<core::graphics::SemaphoreInfo> gate;
    };

    void initListeners(core::CommandBus &bus);
    void initListeners(common::EventBus &bus);
    void cleanupListeners(core::CommandBus &bus);
    void cleanupListeners(common::EventBus &bus);
    void queueRetire(const Handle &resource, std::optional<core::graphics::SemaphoreInfo> gate);
    /// @brief Build and bind the new resource, then retire the previous one.
    void commitSwap(SwapTransaction &transaction);
    /// @brief Append a committed swap to the reusable dispatch buffer.
    void recordCommit(const Handle &material, const Handle &newResource);
    /// @brief Emit the batched MaterialSwapCommitted and reset the buffer cursor.
    void flushCommits();
    /// @brief Deletes a rejected incoming resource record. The caller must have made the device idle.
    void deleteIncomingResource(const command::material::set_material_resource::IncomingResource &incoming);
    void finalizeTransactions();
    void drainRetires();

    command::material::SwapFailureHandler m_defaultOnFailure{&MaterialService::TerminateSwap};
    ListenForSetMaterialResource<MaterialService> m_listenForSet;
    ListenForMakeMaterial<MaterialService> m_listenForMake;
    ListenForStartOfNextFrame<MaterialService> m_listenForFrame;
    star::policy::event::ListenForPassCPURecordingWaitResolved<MaterialService> m_listenForWaitResolved;

    core::device::DeviceContext *m_context = nullptr;
    core::device::StarDevice *m_device = nullptr;
    core::device::manager::GraphicsContainer *m_graphicsManagers = nullptr;
    core::CommandBus *m_cmdBus = nullptr;
    common::EventBus *m_evtBus = nullptr;

    std::vector<SwapTransaction> m_transactions;
    std::vector<RetireEntry> m_retires;
    std::vector<MaterialSwapCommitInfo> m_commitBuffer;
    std::size_t m_commitCount{0};
    bool m_listenersInitialized = false;
};
} // namespace star::service
