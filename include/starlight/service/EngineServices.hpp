#pragma once

#include "starlight/service/Service.hpp"

#include <vector>

namespace star::service
{

/// @brief Move-only storage for all services handed to the engine at startup.
class EngineServices
{
  public:
    EngineServices() = default;
    explicit EngineServices(std::vector<Service> services);

    EngineServices(EngineServices &&) = default;
    EngineServices &operator=(EngineServices &&) = default;
    EngineServices(const EngineServices &) = delete;
    EngineServices &operator=(const EngineServices &) = delete;
    ~EngineServices() = default;

    /// Append an application- or mode-specific service.
    void add(Service service);
    void add(std::vector<Service> services);

    /// @brief  Insert a service at the beginning of the initialization order, ahead of all previously added services.
    /// Services are initialized sequentially in list order, and a service which must ANSWER engine-wide commands (e.g.
    /// the frame tracker provider) has tobe initialized before any dependent service resolves those commands.
    /// @param service New service
    void addFirst(Service service);

    /// @brief Hand ownership of all contained services to the caller. May only be called once; a second call throws.
    /// @return
    std::vector<Service> takeAll();

    size_t size() const;
    bool empty() const;

  private:
    std::vector<Service> m_services;
    bool m_taken{false};
};
} // namespace star::service
