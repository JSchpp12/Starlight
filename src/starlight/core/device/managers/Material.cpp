#include "device/managers/Material.hpp"

#include "virtual/StarMaterial.hpp"

namespace star::core::device::manager
{
void MaterialRecord::cleanupRender(device::StarDevice &device)
{
    (void)device;
}

Material::Material() : Base(GetMaterialEventTypeName)
{
}

Handle Material::getResourceHandle(const Handle &material) const
{
    const auto *record = get(material);
    if (record == nullptr || record->material == nullptr)
        return Handle();

    return record->material->getResourceHandle();
}

MaterialRecord Material::createRecord(MaterialRequest &&request) const
{
    return MaterialRecord{std::move(request.material)};
}
} // namespace star::core::device::manager
