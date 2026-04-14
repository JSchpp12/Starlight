#include "starlight/service/detail/scene_loader/ObjectWriter.hpp"

#include "starlight/service/detail/scene_loader/ObjectUtils.hpp"
#include "starlight/core/json/glm_json.hpp"

namespace star::service::scene_loader
{
nlohmann::json ObjectWriter::write(const StarObject &object) const noexcept
{
    nlohmann::json j;

    const auto pos = object.getInstance().getPosition();
    const auto rotDeg = ExtractRotationDegrees(object);
    const auto scale = object.getInstance().getScale();

    j["position"] = pos;
    j["rotation_deg"] = rotDeg;
    j["scale"] = scale;

    return j;
}
} // namespace star::service::scene_loader