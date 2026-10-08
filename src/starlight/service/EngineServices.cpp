#include "starlight/service/EngineServices.hpp"

#include "starlight/core/Exceptions.hpp"

#include <utility>

namespace star::service
{
EngineServices::EngineServices(std::vector<Service> services) : m_services(std::move(services))
{
}

void EngineServices::add(Service service)
{
    m_services.emplace_back(std::move(service));
}

void EngineServices::add(std::vector<Service> services)
{
    for (size_t i{0}; i < services.size(); i++)
    {
        m_services.emplace_back(std::move(services[i]));
    }
}

void EngineServices::addFirst(Service service)
{
    m_services.insert(m_services.begin(), std::move(service));
}

std::vector<Service> EngineServices::takeAll()
{
    if (m_taken)
    {
        STAR_THROW("EngineServices::takeAll() may only be called once");
    }

    m_taken = true;
    return std::move(m_services);
}

size_t EngineServices::size() const
{
    return m_services.size();
}

bool EngineServices::empty() const
{
    return m_services.empty();
}
} // namespace star::service
