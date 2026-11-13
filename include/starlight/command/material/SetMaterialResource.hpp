#pragma once

#include <starlight/core/transfer/ReadinessCondition.hpp>
#include <starlight/exception/SwapException.hpp>
#include <starlight/setup/RenderResourceSetup.hpp>

#include <star_common/Handle.hpp>
#include <star_common/IServiceCommandWithReply.hpp>

#include <cstdint>
#include <expected>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace star::service
{
class MaterialService;
struct SwapTransaction;
} // namespace star::service

namespace star::command::material
{
namespace set_material_resource
{
inline constexpr const char *GetTypeName()
{
    return "ScMatR";
}

/// @brief The resource being bound and everything that must be ready before it is used.
struct IncomingResource
{
    /// @brief Initialized Image-manager or Buffer-manager handle. The handle type is the resource kind.
    Handle handle;
    /// @brief All dependencies: transfer conditions carry `timelineWait`; pipeline/shader conditions do not.
    std::vector<ReadinessCondition> readiness;
};
} // namespace set_material_resource

/// @brief Which material binding is being swapped.
struct MaterialTarget
{
    Handle material;
    size_t setIndex{0};
    size_t bindingIndex{0};
    std::optional<size_t> frameInFlightIndex;
};

/// @brief Function executed when the swap fails. A raw function pointer; the handler receives the service and record.
using SwapFailureHandler = void (*)(service::MaterialService &, service::SwapTransaction &);

/// @brief Successful swap request state. The material commits when the owning pass confirms readiness.
enum class SwapStatus : std::uint8_t
{
    Pending
};

/// @brief Retarget a material's primary resource and carry the one-shot work and readiness it needs.
/// @details The material keeps its previous binding until the owning pass confirms the swap. The service records a
/// transaction and creates/binds the new StarShaderInfo only on commit.
struct SetMaterialResource
    : public star::common::IServiceCommandWithReply<std::expected<SwapStatus, exception::SwapException>>
{
    static inline constexpr std::string_view GetUniqueTypeName()
    {
        return set_material_resource::GetTypeName();
    }

    SetMaterialResource(MaterialTarget target, Handle recordingCommandBuffer,
                        set_material_resource::IncomingResource incoming, setup::RenderResourceSetup::PrepRequest prep,
                        SwapFailureHandler onFailure = nullptr)
        : target(std::move(target)), recordingCommandBuffer(std::move(recordingCommandBuffer)),
          incoming(std::move(incoming)), prep(std::move(prep)), onFailure(onFailure)
    {
    }

    MaterialTarget target;
    /// @brief Command buffer of the render phase that owns the resource being swapped.
    Handle recordingCommandBuffer;
    set_material_resource::IncomingResource incoming;
    /// @brief One-shot work for the new resource, prepared by the service and owned by the pass.
    setup::RenderResourceSetup::PrepRequest prep;
    /// @brief Executed on a failed swap. Null uses the service default.
    SwapFailureHandler onFailure{nullptr};
};
} // namespace star::command::material
