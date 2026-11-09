#include "device/managers/Material.hpp"

#include "virtual/StarMaterial.hpp"

namespace star::core::device::manager
{
MaterialRequest::MaterialRequest(std::unique_ptr<star::StarMaterial> value) : material(std::move(value))
{
}
MaterialRequest::MaterialRequest(MaterialRequest &&other) noexcept = default;
MaterialRequest &MaterialRequest::operator=(MaterialRequest &&other) noexcept = default;
MaterialRequest::~MaterialRequest() = default;

MaterialRecord::MaterialRecord() = default;

MaterialRecord::MaterialRecord(std::unique_ptr<star::StarMaterial> value) : material(std::move(value))
{
}
MaterialRecord::MaterialRecord(MaterialRecord &&other) noexcept = default;
MaterialRecord &MaterialRecord::operator=(MaterialRecord &&other) noexcept = default;
MaterialRecord::~MaterialRecord() = default;

void MaterialRecord::cleanupRender(device::StarDevice &device)
{
    (void)device;
}

Material::Material() : Base(GetMaterialEventTypeName)
{
}

MaterialRecord Material::createRecord(MaterialRequest &&request) const
{
    return MaterialRecord{std::move(request.material)};
}
} // namespace star::core::device::manager
