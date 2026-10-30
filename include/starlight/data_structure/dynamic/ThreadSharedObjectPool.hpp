#pragma once

#include "core/Exceptions.hpp"
#include "core/device/DeviceContext.hpp"

#include <star_common/Handle.hpp>

#include <boost/lockfree/queue.hpp>

#include <concepts>
#include <vector>

namespace star::data_structure::dynamic
{
template <typename TMaker, typename TObject>
concept MakerLike = requires(TMaker maker) {
    { maker() } -> std::same_as<TObject>;
};

template <typename TObject, MakerLike<TObject> TMaker, size_t TCapacity> class ThreadSharedObjectPool
{
  public:
    explicit ThreadSharedObjectPool(TMaker maker)
        : m_objects(TCapacity), m_created(TCapacity, false), m_maker(std::move(maker)), m_available()
    {
        for (uint32_t i = 0; i < TCapacity; ++i)
        {
            m_available.push(Handle{.type = 0, .id = i});
        }
    }

    Handle acquireBlocking()
    {
        Handle acquired;

        bool printed{false};
        while (!m_available.pop(acquired))
        {
            if (!printed)
            {
                core::logging::log(boost::log::trivial::info, "Blocking acquire call made. Thread yielding");
                printed = true;
            }

            std::this_thread::yield();
        }

        ensureCreated(acquired.getID());
        return acquired;
    }

    TObject &get(const Handle &handle)
    {
        assert(handle.getID() < TCapacity);
        return m_objects[handle.getID()];
    }

    const TObject &get(const Handle &handle) const
    {
        assert(handle.getID() < TCapacity);
        return m_objects[handle.getID()];
    }

    void release(Handle handle)
    {
        assert(handle.getID() < TCapacity);
        if (!m_available.push(std::move(handle)))
        {
            STAR_THROW("Release call failed to push available space");
        }
    }

    static constexpr size_t capacity()
    {
        return TCapacity;
    }

    std::vector<TObject> &getRayObjects()
    {
        return m_objects;
    }

  private:
    std::vector<TObject> m_objects;
    std::vector<bool> m_created;
    TMaker m_maker;
    boost::lockfree::queue<Handle, boost::lockfree::capacity<TCapacity>> m_available;

    void ensureCreated(const uint32_t &idx)
    {
        if (!m_created[idx])
        {
            m_objects[idx] = m_maker();
            m_created[idx] = true;
        }
    }
};
} // namespace star::data_structure::dynamic