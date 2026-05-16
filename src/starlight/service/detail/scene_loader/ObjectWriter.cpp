#include "starlight/service/detail/scene_loader/ObjectWriter.hpp"

#include "starlight/core/helper/star_object/ObjectHelpers.hpp"
#include "starlight/core/json/glm_json.hpp"

namespace star::service::scene_loader
{
nlohmann::json ObjectWriter::write(const StarObject &object) const noexcept
{
    nlohmann::json j;

    const auto pos = object.getInstance().getPosition();
    const auto rotDeg = core::helper::star_object::ExtractRotationDegrees(object.getInstance().getRotationMat());
    const auto scale = object.getInstance().getScale();

    j["position"] = pos;
    j["rotation_deg"] = rotDeg;
    j["scale"] = scale;

    return j;
}
} // namespace star::service::scene_loader