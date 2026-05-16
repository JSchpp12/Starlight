#include "starlight/service/detail/scene_loader/ObjectReader.hpp"

#include "starlight/core/json/glm_json.hpp"

#include "starlight/core/helper/star_object/ObjectHelpers.hpp"

namespace star::service::scene_loader
{

bool ObjectLoader::canLoad(const nlohmann::json &objectDatas, const std::string &uniqueName) const noexcept
{
    if (!objectDatas.contains(uniqueName))
    {
        return false;
    }

    const auto &jData = objectDatas[uniqueName];
    return jData.contains("position") && jData.contains("scale") && jData.contains("rotation_deg");
}

void ObjectLoader::load(const nlohmann::json &objectDatas, const std::string &uniqueName,
                        StarObject &obj) const noexcept
{
    if (!objectDatas.contains(uniqueName))
    {
        return;
    }

    auto &jData = objectDatas[uniqueName];
    glm::vec3 position = jData["position"];
    glm::vec3 scale = jData["scale"];
    glm::vec3 rotation = jData["rotation_deg"];

    const auto rotToApply = core::helper::star_object::ConvertFromEulerToGlobalRotations(rotation);

    obj.getInstance().setPosition(position);
    obj.getInstance().setScale(scale);
    for (const auto &rot : rotToApply)
    {
        obj.getInstance().rotateRelative(rot.first, rot.second, true);
    }
}
} // namespace star::service::scene_loader