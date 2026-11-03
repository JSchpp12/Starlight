#include "starlight/structs/PngData.hpp"

#include "stb_image.h"

#include <utility>

namespace star
{
PngData::PngData(unsigned char *data, std::size_t size) noexcept : m_data(data), m_size(size)
{
}

PngData::~PngData()
{
    release();
}

PngData::PngData(PngData &&other) noexcept
    : m_data(std::exchange(other.m_data, nullptr)), m_size(std::exchange(other.m_size, 0))
{
}

PngData &PngData::operator=(PngData &&other) noexcept
{
    if (this != &other)
    {
        release();
        m_data = std::exchange(other.m_data, nullptr);
        m_size = std::exchange(other.m_size, 0);
    }

    return *this;
}

void PngData::release() noexcept
{
    if (m_data)
    {
        stbi_image_free(m_data);
        m_data = nullptr;
    }

    m_size = 0;
}
} // namespace star
