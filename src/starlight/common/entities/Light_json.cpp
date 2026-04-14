#include "starlight/common/entities/Light_json.hpp"

#include "starlight/core/json/glm_json.hpp"

void star::to_json(nlohmann::json &j, const Light &light)
{
    j = nlohmann::json{// StarEntity base field
                       {"position", light.getPosition()},

                       // Light-specific fields
                       {"direction", light.getDirection()},
                       {"ambient", light.getAmbient()},
                       {"diffuse", light.getDiffuse()},
                       {"specular", light.getSpecular()},
                       {"innerDiameter", light.getInnerDiameter()},
                       {"outerDiameter", light.getOuterDiameter()},
                       {"luminance", light.getLuminance()},
                       {"type", light.getType()},
                       {"enabled", light.getEnabled()}};
}

void star::from_json(const nlohmann::json &j, Light &light)
{
    // StarEntity base field
    if (j.contains("position"))
        light.setPosition(j.at("position").get<glm::vec3>());

    // Light-specific fields
    if (j.contains("direction"))
        light.setDirection(j.at("direction").get<glm::vec3>());

    if (j.contains("ambient"))
        light.setAmbient(j.at("ambient").get<glm::vec3>());

    if (j.contains("diffuse"))
        light.setDiffuse(j.at("diffuse").get<glm::vec3>());

    if (j.contains("specular"))
        light.setSpecular(j.at("specular").get<glm::vec3>());

    if (j.contains("innerDiameter"))
        light.setInnerDiameter(j.at("innerDiameter").get<float>());

    if (j.contains("outerDiameter"))
        light.setOuterDiameter(j.at("outerDiameter").get<float>());

    if (j.contains("luminance"))
        light.setLuminance(j.at("luminance").get<uint32_t>());

    if (j.contains("type"))
        light.setType(j.at("type").get<Type::Light>());

    if (j.contains("enabled"))
        light.setEnabled(j.at("enabled").get<bool>());
}
