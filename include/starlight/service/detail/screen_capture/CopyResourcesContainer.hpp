#pragma once

#include "StarBuffers/Buffer.hpp"
#include "data_structure/dynamic/ThreadSharedObjectPool.hpp"
#include "starlight/maker/buffer/MakeBuffer.hpp"

namespace star::service::detail::screen_capture
{
class CopyResourcesContainer
{
  public:
    explicit CopyResourcesContainer(star::maker::buffer::MakeBuffer maker) : m_hostVisibleBufferPool(std::move(maker))
    {
    }

    data_structure::dynamic::ThreadSharedObjectPool<star::StarBuffers::Buffer, star::maker::buffer::MakeBuffer, 50> &
    getBufferPool()
    {
        return m_hostVisibleBufferPool;
    }

  private:
    data_structure::dynamic::ThreadSharedObjectPool<star::StarBuffers::Buffer, star::maker::buffer::MakeBuffer, 50>
        m_hostVisibleBufferPool;
};
} // namespace star::service::detail::screen_capture
