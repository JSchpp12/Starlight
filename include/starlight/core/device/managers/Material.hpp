#pragma once

#include "device/managers/ManagerEventBusTies.hpp"
#include "virtual/StarMaterial.hpp"

#include <star_common/special_types/SpecialHandleTypes.hpp>

#include <memory>
#include <string_view>
#include <utility>

namespace star::core::device::manager
{
/// @brief Event-bus binding for the material manager. Distinct from the "stMat" handle type.
constexpr std::string_view GetMaterialEventTypeName = "stMatE";

/// @brief Compile-time name of the material resource type, used for handle registration and failure logging.
/// @details Reuses the already-registered `"stMat"` handle type (HandleTypeRegistry::initSpecialTypes).
struct MaterialResourceTypeName
{
    static constexpr std::string_view GetUniqueTypeName()
    {
        return common::special_types::MaterialTypeName;
    }
};

struct MaterialRequest
{
    std::unique_ptr<star::StarMaterial> material;

    explicit MaterialRequest(std::unique_ptr<star::StarMaterial> value) : material(std::move(value))
    {
    }
    MaterialRequest(const MaterialRequest &) = delete;
    MaterialRequest &operator=(const MaterialRequest &) = delete;
    MaterialRequest(MaterialRequest &&other) noexcept = default;
    MaterialRequest &operator=(MaterialRequest &&other) noexcept = default;
    ~MaterialRequest() = default;
};

struct MaterialRecord
{
    std::unique_ptr<star::StarMaterial> material;

    MaterialRecord() = default;
    explicit MaterialRecord(std::unique_ptr<star::StarMaterial> value) : material(std::move(value))
    {
    }
    MaterialRecord(const MaterialRecord &) = delete;
    MaterialRecord &operator=(const MaterialRecord &) = delete;
    MaterialRecord(MaterialRecord &&other) noexcept = default;
    MaterialRecord &operator=(MaterialRecord &&other) noexcept = default;
    ~MaterialRecord() = default;

    /// @brief Step 1 of the material-manager migration: objects still run the material's GPU cleanup.
    /// @details The manager is the owner of the material object; authoritative cleanup moves here once the
    /// non-virtual material rework replaces StarMaterial.
    void cleanupRender(device::StarDevice &device);
};

/// @brief Addressable storage for materials, analogous to the Image/Buffer managers.
/// @details The handle type is `"stMat"`; the event type is `"stMatE"`. `getMaterial` is a convenience over
/// `get(handle)->material` for call sites that only need the material itself.
class Material : public ManagerEventBusTies<MaterialRecord, MaterialRequest, 30, MaterialResourceTypeName>
{
  public:
    using Base = ManagerEventBusTies<MaterialRecord, MaterialRequest, 30, MaterialResourceTypeName>;

    Material();
    Material(const Material &) = delete;
    Material &operator=(const Material &) = delete;
    Material(Material &&other) = default;
    Material &operator=(Material &&other) = default;

    virtual ~Material() = default;

    using Base::get;

    Handle getResourceHandle(const Handle &material) const;

    star::StarMaterial *getMaterial(const Handle &handle)
    {
        auto *record = get(handle);
        return record != nullptr ? record->material.get() : nullptr;
    }

    const star::StarMaterial *getMaterial(const Handle &handle) const
    {
        const auto *record = get(handle);
        return record != nullptr ? record->material.get() : nullptr;
    }

  private:
    virtual MaterialRecord createRecord(MaterialRequest &&request) const override;
};
} // namespace star::core::device::manager
