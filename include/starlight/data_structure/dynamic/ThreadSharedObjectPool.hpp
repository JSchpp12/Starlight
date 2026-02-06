#pragma once

#include "core/Exceptions.hpp"
#include "core/device/DeviceContext.hpp"

#include <star_common/Handle.hpp>

#include <boost/lockfree/queue.hpp>

#include <concepts>
#include <vector>

namespace star::data_structure::dynamic
{
template <typename TCreatePolicy, typename TObject>
concept CreatePolicyLike = requires(TCreatePolicy c) {
    { c.create() } -> std::same_as<TObject>;
};

template <typename TObject, CreatePolicyLike<TObject> TCreatePolicy, size_t TCapacity> class ThreadSharedObjectPool
{
  public:
    explicit ThreadSharedObjectPool(TCreatePolicy createPolicy)
        : m_objects(TCapacity), m_created(TCapacity, false), m_createPolicy(std::move(createPolicy)),
          m_available(TCapacity)
    {
        for (uint32_t i = 0; i < TCapacity; ++i)
        {
            m_available.push(Handle{.type = 0, .id = i});
        }
    }

    Handle acquireBlocking()
    {
        Handle acquired;

        int lockCounter = 0;
        while (!m_available.pop(acquired))
        {
            lockCounter++;
            std::this_thread::yield();

            if (lockCounter == 50)
            {
                core::logging::log(boost::log::trivial::info, "Blocking acquire call made. Thread yielding");
                lockCounter = 0;
            }
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
    TCreatePolicy m_createPolicy;
    boost::lockfree::queue<Handle, boost::lockfree::capacity<TCapacity>> m_available;

    void ensureCreated(const uint32_t &idx)
    {
        if (!m_created[idx])
        {
            m_objects[idx] = m_createPolicy.create();
            m_created[idx] = true;
        }
    }
};
} // namespace star::data_structure::dynamic