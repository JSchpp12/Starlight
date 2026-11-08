#pragma once

#include "ManagedHandleContainer.hpp"
#include "ManagerBase.hpp"
#include "core/exceptions/ResourceCreationError.hpp"
#include "core/logging/LoggingFactory.hpp"
#include "device/StarDevice.hpp"
#include "job/TaskManager.hpp"

#include <star_common/Handle.hpp>

#include <array>
#include <cassert>
#include <chrono>
#include <concepts>
#include <expected>
#include <future>
#include <optional>
#include <stack>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

namespace star::core::device::manager
{
/// @brief Value a producer publishes for a manager-owned resource: the resource, or the error.
template <typename TResource> using PendingResult = std::expected<TResource, core::ResourceCreationError>;

/// @brief Future-backed producer side for a manager record.
template <typename TResource> class PendingResource
{
  public:
    using Resource = TResource;
    using Result = PendingResult<TResource>;

    explicit PendingResource(std::future<Result> future) : m_future(std::move(future))
    {
    }

    PendingResource(PendingResource &&) noexcept = default;
    PendingResource &operator=(PendingResource &&) noexcept = default;
    PendingResource(const PendingResource &) = delete;
    PendingResource &operator=(const PendingResource &) = delete;

    /// @brief Non-blocking check for a published result.
    bool isReady() const noexcept
    {
        // wait_for() is undefined on an invalid future (e.g. one already consumed or created from a broken promise).
        return m_future.valid() && m_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }

    bool isValid() const noexcept
    {
        return m_future.valid();
    }

    /// @brief Block until the producer publishes (or the future is otherwise resolved).
    void wait() const
    {
        if (m_future.valid())
            m_future.wait();
    }

    /// @brief Consume the published result. Never throws: a producer destroyed without publishing is reported as an
    /// unexpected so the caller can fail soft instead of waiting forever.
    Result take() noexcept
    {
        try
        {
            return m_future.get();
        }
        catch (const std::exception &e)
        {
            return std::unexpected(core::ResourceCreationError{std::string{"resource producer failed: "} + e.what()});
        }
    }

  private:
    std::future<Result> m_future;
};

/// @brief Compile-time provider for the name of a manager's resource type (e.g. its registered handle type name).
/// @details Managers and their slots carry this as a template parameter so a failed producer can be named without
/// storing the name or threading it through the record lifecycle.
template <typename T>
concept ResourceTypeNameProvider = requires {
    { T::GetUniqueTypeName() } -> std::convertible_to<std::string_view>;
};

/// @brief A manager record that may be published asynchronously.
/// @details A request carries either the record's resource or a future for it; the slot stores the record and converts
/// the published resource into the record when the future resolves.
template <typename TRecord, ResourceTypeNameProvider TResourceTypeName, typename TResource = TRecord> class ResourceSlot
{
  public:
    using Record = TRecord;
    using Resource = TResource;
    using Result = PendingResult<TResource>;

    /// @brief Empty slot used by the container's growth path before a record is inserted or committed.
    ResourceSlot() = default;
    explicit ResourceSlot(TRecord record) : m_value(std::move(record))
    {
    }
    explicit ResourceSlot(std::future<Result> future) : m_pending(PendingResource<TResource>{std::move(future)})
    {
    }
    explicit ResourceSlot(std::variant<TResource, std::future<Result>> payload)
    {
        if (auto *resource = std::get_if<TResource>(&payload))
        {
            m_value.emplace(std::move(*resource));
        }
        else
        {
            m_pending.emplace(std::get<std::future<Result>>(std::move(payload)));
        }
    }
    ResourceSlot(ResourceSlot &&) noexcept = default;
    ResourceSlot &operator=(ResourceSlot &&) noexcept = default;
    ResourceSlot(const ResourceSlot &) = delete;
    ResourceSlot &operator=(const ResourceSlot &) = delete;

    /// @brief True once a concrete record is present, including the blank record used when a producer failed.
    bool isReady() const noexcept
    {
        return m_value.has_value() || (m_pending.has_value() && m_pending->isReady());
    }

    bool isPending() const noexcept
    {
        return m_pending.has_value();
    }

    /// @brief Block until a pending producer publishes, or return immediately when nothing is pending.
    void wait() const
    {
        if (m_pending.has_value())
            m_pending->wait();
    }

    TRecord *get() noexcept
    {
        return m_value ? &*m_value : nullptr;
    }
    const TRecord *get() const noexcept
    {
        return m_value ? &*m_value : nullptr;
    }

    /// @brief Consume a ready future into the concrete slot. Never blocks.
    void resolve() const
    {
        if (!m_pending.has_value() || !m_pending->isReady())
        {
            return;
        }

        auto result = m_pending->take();
        m_pending.reset();

        if (result.has_value())
        {
            m_value.emplace(std::move(*result));
            return;
        }

        core::logging::warning(TResourceTypeName::GetUniqueTypeName(), " manager: resource producer failed (",
                               result.error().what(), ")");
        if constexpr (std::is_default_constructible_v<TRecord>)
        {
            m_value.emplace();
        }
    }

    void cleanupRender(device::StarDevice &device)
    {
        if (m_pending.has_value())
        {
            // A slot still unpublishing at teardown owns nothing from the manager's perspective. Its producer must
            // publish (and clean up) before the device goes away; blocking here would hide that ownership mistake.
            if (!m_pending->isReady())
            {
                return;
            }
            resolve();
        }

        if (m_value.has_value())
        {
            m_value->cleanupRender(device);
        }
    }

  private:
    mutable std::optional<TRecord> m_value;
    mutable std::optional<PendingResource<TResource>> m_pending;
};

/// @brief Standard request payload for a manager whose resource may be published asynchronously.
/// @details Child managers derive their request from this (or use it directly) and add only their own extra
/// constructors; the base manager turns the payload into a resolved-or-pending slot.
template <typename TRecord> struct ManagerResourceRequest
{
    using Resource = TRecord;

    std::variant<TRecord, std::future<PendingResult<TRecord>>> payload;

    explicit ManagerResourceRequest(TRecord record) : payload(std::move(record))
    {
    }
    explicit ManagerResourceRequest(std::future<PendingResult<TRecord>> future) : payload(std::move(future))
    {
    }
};

/// @brief The resource a request publishes, or the record type itself when the request is always created synchronously.
template <typename TRequest, typename TRecord> struct StoredResourceType
{
    using type = TRecord;
};
template <typename TRequest, typename TRecord>
    requires requires { typename TRequest::Resource; }
struct StoredResourceType<TRequest, TRecord>
{
    using type = typename TRequest::Resource;
};

template <typename TRecord, typename TResourceRequest, size_t TMaxRecordCount,
          ResourceTypeNameProvider TResourceTypeName>
class Manager : public ManagerBase<TRecord, TResourceRequest>
{
  public:
    /// @brief What a request publishes (a resource or a future for one), converted into TRecord when it resolves.
    using StoredResource = typename StoredResourceType<TResourceRequest, TRecord>::type;

    Manager()
        : ManagerBase<TRecord, TResourceRequest>(TResourceTypeName::GetUniqueTypeName()),
          m_records(TResourceTypeName::GetUniqueTypeName(), TMaxRecordCount, 25) {};

    virtual ~Manager() = default;
    Manager(const Manager &) = delete;
    Manager &operator=(const Manager &) = delete;
    Manager(Manager &&other) noexcept
        : ManagerBase<TRecord, TResourceRequest>(std::move(other)), m_records(std::move(other.m_records)) {};
    Manager &operator=(Manager &&other) noexcept
    {
        if (this != &other)
        {
            ManagerBase<TRecord, TResourceRequest>::operator=(std::move(other));

            m_records = std::move(other.m_records);
        }

        return *this;
    }

    virtual Handle submit(TResourceRequest resource) override
    {
        assert(this->m_device != nullptr && "Init must be called first");

        return insert(std::move(resource));
    }

    /// @brief Resolve a ready future and return the concrete record, or nullptr while it is still pending.
    virtual TRecord *get(const Handle &handle)
    {
        auto &slot = m_records.get(handle);
        slot.resolve();
        return slot.get();
    }

    virtual const TRecord *get(const Handle &handle) const
    {
        const auto &slot = m_records.get(handle);
        slot.resolve();
        return slot.get();
    }

    /// @brief True once the record is concrete (a ready future counts as ready before it is consumed).
    bool isReady(const Handle &handle) const
    {
        return m_records.get(handle).isReady();
    }

    /// @brief Block until a pending resource is published, then resolve it into the record.
    void waitForReady(const Handle &handle)
    {
        auto &slot = m_records.get(handle);
        slot.wait();
        slot.resolve();
    }

    auto &getRecords()
    {
        return m_records;
    }

    virtual void cleanupRender()
    {
        if (this->m_device != nullptr)
        {
            m_records.cleanupAll(this->m_device);
        }
    }

    void deleteRequest(device::StarDevice &device, const Handle &requestHandle)
    {
        assert(requestHandle.getType() == this->getHandleType());
        m_records.remove(requestHandle, &device);
    }

  protected:
    star::core::ManagedHandleContainer<ResourceSlot<TRecord, TResourceTypeName, StoredResource>> m_records;

    Handle insert(TResourceRequest request)
    {
        return m_records.insert(createSlot(std::move(request)));
    }

    /// @brief Build the stored slot for a request.
    /// @details Requests derived from ManagerResourceRequest carry either the published resource or a future for it;
    /// the slot stores the record and converts the resource into it when it arrives. Every other request is created
    /// synchronously through createRecord(). Child managers provide only the types.
    virtual ResourceSlot<TRecord, TResourceTypeName, StoredResource> createSlot(TResourceRequest &&request) const
    {
        if constexpr (std::derived_from<TResourceRequest, ManagerResourceRequest<StoredResource>>)
        {
            return ResourceSlot<TRecord, TResourceTypeName, StoredResource>{std::move(request.payload)};
        }
        else
        {
            return ResourceSlot<TRecord, TResourceTypeName, StoredResource>{createRecord(std::move(request))};
        }
    }

    virtual TRecord createRecord(TResourceRequest &&request) const = 0;
};
} // namespace star::core::device::manager
