#pragma once

#include <memory>

#include "InitParameters.hpp"
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
        virtual void doSetInitParameters(InitParameters &params) = 0;
        virtual void doInit(const uint8_t &numFramesInFlight) = 0;
        virtual void doShutdown() = 0;
    };

    template <typename TService> struct ServiceModel : public ServiceConcept
    {
        TService m_service;
        ServiceModel(TService service) : m_service(std::move(service))
        {
        }
        virtual ~ServiceModel() = default;

        void doSetInitParameters(InitParameters &params) override
        {
            m_service.setInitParameters(params);
        }
        
        void doInit(const uint8_t &numFramesInFlight) override
        {
            m_service.init(numFramesInFlight);
        }

        void doShutdown() override
        {
            m_service.shutdown();
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

    void init(InitParameters &params, const uint8_t &numFramesInFlight)
    {
        setInitParameters(params);
        m_impl->doInit(numFramesInFlight);
    }

    void setInitParameters(InitParameters &params){
        m_impl->doSetInitParameters(params);
    }

    void shutdown()
    {
        m_impl->doShutdown();
    }
};
} // namespace star::service