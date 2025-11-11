#pragma once

#include <memory>

#include "core/device/managers/GraphicsContainer.hpp"
#include "core/device/system/EventBus.hpp"
#include "job/TaskManager.hpp"

namespace star::service
{
class Service
{
  private:
    struct ServiceConcept
    {
        virtual ~ServiceConcept() = default;
        virtual void doInit(const Handle &deviceID, core::device::system::EventBus &eventBus,
                            job::TaskManager &taskManager,
                            core::device::manager::GraphicsContainer &graphicsContainer) = 0;
        virtual void doShutdown(const Handle &deviceID, core::device::system::EventBus &eventBus,
                                job::TaskManager &taskManager,
                                core::device::manager::GraphicsContainer &graphicsContainer) = 0;
    };

    template <typename TService> struct ServiceModel : public ServiceConcept
    {
        TService m_service;
        ServiceModel(TService service) : m_service(std::move(service))
        {
        }
        virtual ~ServiceModel() = default;

        void doInit(const Handle &deviceID, core::device::system::EventBus &eventBus, job::TaskManager &taskManager,
                    core::device::manager::GraphicsContainer &graphicsResources) override
        {
            m_service.init(deviceID, eventBus, taskManager, graphicsResources);
        }

        void doShutdown(const Handle &deviceID, core::device::system::EventBus &eventBus, job::TaskManager &taskManager,
                        core::device::manager::GraphicsContainer &graphicsResources) override
        {
            m_service.shutdown(deviceID, eventBus, taskManager, graphicsResources);
        }
    };

    std::unique_ptr<ServiceConcept> m_impl;

  public:
    template <typename TService>
    Service(TService service) : m_impl(std::make_unique<ServiceModel<TService>>(std::move(service)))
    {
    }
    Service(const Service &) = delete;
    Service &operator=(const Service &) = delete;
    Service(Service &&) = default;
    Service &operator=(Service &&) = default;
    ~Service() = default;

    void init(const Handle &deviceID, core::device::system::EventBus &eventBus, job::TaskManager &taskManager,
              core::device::manager::GraphicsContainer &graphicsResources)
    {
        m_impl->doInit(deviceID, eventBus, taskManager, graphicsResources);
    }

    void shutdown(const Handle &deviceID, core::device::system::EventBus &eventBus, job::TaskManager &taskManager,
                  core::device::manager::GraphicsContainer &graphicsResources)
    {
        m_impl->doShutdown(deviceID, eventBus, taskManager, graphicsResources);
    }
};
} // namespace star::service