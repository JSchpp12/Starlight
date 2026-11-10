#pragma once

#include "core/graphics/GPUWorkSyncInfo.hpp"
#include "starlight/command/material/CreateMaterial.hpp"
#include "starlight/command/material/SetMaterialResource.hpp"
#include "starlight/event/StartOfNextFrame.hpp"
#include "starlight/policy/command/ListenFor.hpp"
#include "starlight/policy/event/ListenFor.hpp"
#include "starlight/core/WorkerPool.hpp"
#include "starlight/service/InitParameters.hpp"

#include <star_common/Handle.hpp>

#include <vector>

namespace star::core::device
{
class DeviceContext;
}

namespace star::service
{
template <typename T>
using ListenForCreateMaterial =
    star::policy::command::ListenFor<T, star::command::material::CreateMaterial,
                                     star::command::material::create_material::GetTypeName, &T::onCreateMaterial>;
template <typename T>
using ListenForSetMaterialResource =
    star::policy::command::ListenFor<T, star::command::material::SetMaterialResource,
                                     star::command::material::set_material_resource::GetTypeName,
                                     &T::onSetMaterialResource>;
template <typename T>
using ListenForStartOfNextFrame =
    star::policy::event::ListenFor<T, star::event::StartOfNextFrame,
                                   star::event::start_of_next_frame::GetUniqueTypeName,
                                   &T::onStartOfNextFrame>;

/// @brief Material lifecycle service: creation, resource switching, and retirement.
/// @details Basic implementation: the swap commits in the command handler. Recording the one-shot
/// RenderResourceSetup through CommandOrderService is the next step; until then the setup is retained and cleaned at
/// shutdown.
class MaterialService
{
  public:
    MaterialService() : m_listenForCreate(*this), m_listenForSet(*this), m_listenForFrame(*this)
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

    void onCreateMaterial(command::material::CreateMaterial &cmd);
    void onSetMaterialResource(command::material::SetMaterialResource &cmd);
    void onStartOfNextFrame(const event::StartOfNextFrame &event, bool &keepAlive);

  private:
    struct RetireEntry
    {
        Handle handle;
        bool isImage{false};
        core::graphics::SemaphoreInfo gate;
    };

    void initListeners(core::CommandBus &bus);
    void initListeners(common::EventBus &bus);
    void cleanupListeners(core::CommandBus &bus);
    void cleanupListeners(common::EventBus &bus);
    void drainRetires();

    ListenForCreateMaterial<MaterialService> m_listenForCreate;
    ListenForSetMaterialResource<MaterialService> m_listenForSet;
    ListenForStartOfNextFrame<MaterialService> m_listenForFrame;

    core::device::DeviceContext *m_context = nullptr;
    core::device::StarDevice *m_device = nullptr;
    core::device::manager::GraphicsContainer *m_graphicsManagers = nullptr;
    core::CommandBus *m_cmdBus = nullptr;
    common::EventBus *m_evtBus = nullptr;

    std::vector<RetireEntry> m_retires;
    std::vector<setup::RenderResourceSetup> m_orphanedSetups;
    bool m_listenersInitialized = false;
};
} // namespace star::service
