#pragma once

#include "InitParameters.hpp"
#include "core/device/managers/GraphicsContainer.hpp"
#include "job/TaskManager.hpp"

#include <memory>
#include <star_common/EventBus.hpp>
#include <star_common/IServiceCommand.hpp>

namespace star::service
{
class Service
{
  private:
    struct ServiceConcept
    {
        virtual ~ServiceConcept() = default;
        virtual void doSetInitParameters(InitParameters &params) = 0;
        virtual void doInit() = 0;
        virtual void doShutdown() = 0;
    };

    template <typename TService> struct ServiceModel : public ServiceConcept
    {
        TService m_service;

        template <typename UService>
        explicit ServiceModel(UService &&service) : m_service(std::forward<UService>(service))
        {
        }

        virtual ~ServiceModel() = default;

        void doSetInitParameters(InitParameters &params) override
        {
            m_service.setInitParameters(params);
        }

        void doInit() override
        {
            m_service.init();
        }

        void doShutdown() override
        {
            m_service.shutdown();
        }
    };

    std::unique_ptr<ServiceConcept> m_impl;

  public:
    template <typename TService>
    explicit Service(TService &&service)
        : m_impl(std::make_unique<ServiceModel<std::decay_t<TService>>>(std::forward<TService>(service)))
    {
    }
    Service(const Service &) = delete;
    Service &operator=(const Service &) = delete;
    Service(Service &&) = default;
    Service &operator=(Service &&) = default;
    ~Service() = default;

    void init(InitParameters &params)
    {
        setInitParameters(params);
        m_impl->doInit();
    }

    void setInitParameters(InitParameters &params)
    {
        m_impl->doSetInitParameters(params);
    }

    void shutdown()
    {
        m_impl->doShutdown();
    }
};
} // namespace star::service