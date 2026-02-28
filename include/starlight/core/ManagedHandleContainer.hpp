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

template <typename TData, size_t TMaxDataCount>
    requires TDataHasProperCleanup<TData>
class ManagedHandleContainer : public LinearHandleContainer<TData, TMaxDataCount>
{
  public:
    ManagedHandleContainer(std::string_view handleTypeName)
        : LinearHandleContainer<TData, TMaxDataCount>(handleTypeName)
    {
    }
    ManagedHandleContainer(uint16_t registeredHandleType)
        : LinearHandleContainer<TData, TMaxDataCount>(std::move(registeredHandleType))
    {
    }
    virtual ~ManagedHandleContainer() = default;

    void cleanupAll(device::StarDevice *device = nullptr)
    {
        for (uint32_t i = 0; i < this->m_records.size(); i++)
        {
            auto handle = Handle{.type = this->getHandleType(), .id = i};
            cleanup(handle, device);
        }
    }

  protected:
    void removeRecord(const Handle &handle, device::StarDevice *device = nullptr) override
    {
        this->remove(handle, device);

        cleanup(handle, device);
    }
    void cleanup(const Handle &handle, device::StarDevice *device = nullptr)
    {
        assert(handle.getID() < this->m_records.size() &&
               "Handle references location outside of available storage in cleanup");

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