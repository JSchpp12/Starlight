#include "starlight/service/detail/scene_loader/LightWriter.hpp"

#include "starlight/common/entities/Light_json.hpp"

nlohmann::json star::service::scene_loader::LightWriter::write(const std::vector<Light> &light) const noexcept
{
    nlohmann::json jData = light;

    return jData;
}
