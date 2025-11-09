#pragma once

#include <memory>

namespace star::core::service
{
class Service
{
  private:
    struct ServiceConcept
    {
        virtual ~ServiceConcept() = default;
        virtual void doInit() = 0;
        virtual void doShutdown() = 0;
    };

    template <typename TService> struct ServiceModel : public ServiceConcept
    {
        TService m_service;
        ServiceModel(TService service) : m_service(std::move(service))
        {
        }
        virtual ~ServiceModel() = default;

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
    template <typename TService> Service(TService service) : m_impl(std::make_unique<ServiceModel>(std::move(service)))
    {
    }
};
} // namespace star::core::service