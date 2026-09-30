#pragma once

#include "StarBuffers/Buffer.hpp"
#include "data_structure/dynamic/ThreadSharedObjectPool.hpp"
#include "wrappers/graphics/policies/GenericBufferCreateAllocatePolicy.hpp"

namespace star::service::detail::screen_capture
{
class CopyResourcesContainer
{
  public:
    explicit CopyResourcesContainer(
        wrappers::graphics::policies::GenericBufferCreateAllocatePolicy createPolicy)
        : m_hostVisibleBufferPool(std::move(createPolicy))
    {
    }

    data_structure::dynamic::ThreadSharedObjectPool<
        star::StarBuffers::Buffer, wrappers::graphics::policies::GenericBufferCreateAllocatePolicy, 50> &
    getBufferPool()
    {
        return m_hostVisibleBufferPool;
    }

  private:
    data_structure::dynamic::ThreadSharedObjectPool<
        star::StarBuffers::Buffer, wrappers::graphics::policies::GenericBufferCreateAllocatePolicy, 50>
        m_hostVisibleBufferPool;
};
} // namespace star::service::detail::screen_capture
