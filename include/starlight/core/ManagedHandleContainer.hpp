#pragma once

#include "LinearHandleContainer.hpp"

namespace star::core
{
template <typename T>
concept TDataHasCleanup = requires(T record) {
    { record.cleanupRender() } -> std::same_as<void>;
};
template <typename T>
concept TDataHasCleanupRender = requires(T record, device::StarDevice &device) {
    { record.cleanupRender(device) } -> std::same_as<void>;
};
template <typename T>
concept TDataHasVKCleanup = requires(T record, vk::Device &device) {
    { record.cleanupRender(device) } -> std::same_as<void>;
};
template <typename T>
concept TDataHasProperCleanup = TDataHasCleanupRender<T> || TDataHasCleanup<T> || TDataHasVKCleanup<T>;

template <typename TData>
    requires TDataHasProperCleanup<TData>
class ManagedHandleContainer : public LinearHandleContainer<TData>
{
  public:
    ManagedHandleContainer(std::string_view handleTypeName, size_t startCapacity = 0, size_t expandingAmt = 0)
        : LinearHandleContainer<TData>(handleTypeName, startCapacity, expandingAmt)
    {
    }
    ManagedHandleContainer(uint16_t registeredHandleType, size_t startCapacity = 0, size_t expandingAmt = 0)
        : LinearHandleContainer<TData>(std::move(registeredHandleType), startCapacity, expandingAmt)
    {
    }
    virtual ~ManagedHandleContainer() = default;

    void cleanupAll(device::StarDevice *device = nullptr)
    {
        for (uint32_t i = 0; i < this->m_records.size(); i++)
        {
            auto handle = Handle{.type = this->getHandleType(), .id = i};
            if (!this->isFilled(handle))
            {
                continue;
            }
            cleanup(handle, device);
        }
    }

  protected:
    void removeRecord(const Handle &handle, device::StarDevice *device = nullptr) override
    {
        cleanup(handle, device);

        LinearHandleContainer<TData>::removeRecord(handle, device);
    }
    void cleanup(const Handle &handle, device::StarDevice *device = nullptr)
    {
        if (handle.getID() >= this->m_records.size())
        {
            STAR_THROWF("Handle references location outside of available storage in cleanup: id=", handle.getID());
        }

        if (!this->isFilled(handle))
        {
            STAR_THROWF("Handle references a slot that has not been filled in cleanup: id=", handle.getID());
        }

        if constexpr (TDataHasCleanupRender<TData>)
        {
            assert(device != nullptr && "Device must be provided for types which require it in their cleanup");
            this->m_records[handle.getID()].cleanupRender(*device);
        }
        else if constexpr (TDataHasVKCleanup<TData>)
        {
            assert(device != nullptr && "Device must be provided for types which require it in their cleanup");
            this->m_records[handle.getID()].cleanupRender(device->getVulkanDevice());
        }
        else if constexpr (TDataHasCleanup<TData>)
        {
            this->m_records[handle.getID()].cleanupRender();
        }
    }
};
} // namespace star::core