#include "starlight/structs/ExrData.hpp"

#include <cstdlib>
#include <utility>

namespace star
{
ExrData::ExrData(float *data, std::size_t size) noexcept : m_data(data), m_size(size)
{
}

ExrData::~ExrData()
{
    release();
}

ExrData::ExrData(ExrData &&other) noexcept
    : m_data(std::exchange(other.m_data, nullptr)), m_size(std::exchange(other.m_size, 0))
{
}

ExrData &ExrData::operator=(ExrData &&other) noexcept
{
    if (this != &other)
    {
        release();
        m_data = std::exchange(other.m_data, nullptr);
        m_size = std::exchange(other.m_size, 0);
    }

    return *this;
}

void ExrData::release() noexcept
{
    if (m_data)
    {
        std::free(m_data);
        m_data = nullptr;
    }

    m_size = 0;
}
} // namespace star
