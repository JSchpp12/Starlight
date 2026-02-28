#include "starlight/service/detail/scene_loader/SceneFile.hpp"

#include "starlight/core/Exceptions.hpp"

#include <filesystem>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>

namespace star::service::scene_loader
{

std::array<std::pair<star::Type::Axis, float>, 3> ConvertFromEulerToGlobalRotations(const glm::vec3 &rDeg)
{
    float ax = rDeg[0];
    float ay = rDeg[1];
    float az = rDeg[2];
    return std::array<std::pair<star::Type::Axis, float>, 3>{
        std::make_pair(star::Type::Axis::z, az),
        std::make_pair(star::Type::Axis::y, ay),
        std::make_pair(star::Type::Axis::x, ax),
    };
}

glm::vec3 ExtractRotationDegrees(const std::shared_ptr<StarObject> object)
{
    const auto r = glm::mat3(object->getInstance().getRotationMat());

    float sy = -r[0][2];
    float cy = sqrtf(1.0f - sy * sy);

    float x, y, z;

    if (cy > 1e-6f)
    {
        // Standard Y-up Euler XYZ decomposition
        x = atan2f(r[1][2], r[2][2]); // rotX
        y = asinf(sy);                // rotY
        z = atan2f(r[0][1], r[0][0]); // rotZ
    }
    else
    {
        // Gimbal lock case
        x = atan2f(-r[1][0], r[1][1]);
        y = asinf(sy);
        z = 0.0f;
    }

    return glm::degrees(glm::vec3(x, y, z));
}

void SceneFile::write(const SceneObjectTracker &sceneObjects)
{
    // Build JSON tree in memory
    nlohmann::json root = nlohmann::json::object();
    root["Scene"] = nlohmann::json::object();
    root["Scene"]["objects"] = nlohmann::json::object();

    for (const auto &ele : sceneObjects.getStorage())
    {
        const std::string &name = ele.first;
        const auto &obj = ele.second;

        const auto pos = obj->getInstance().getPosition();
        const auto rotDeg = ExtractRotationDegrees(obj);
        const auto scale = obj->getInstance().getScale();

        nlohmann::json jObj = nlohmann::json::object();
        jObj["position"] = {pos.x, pos.y, pos.z};
        jObj["rotation_deg"] = {rotDeg.x, rotDeg.y, rotDeg.z};
        jObj["scale"] = {scale.x, scale.y, scale.z};

        root["Scene"]["objects"][name] = std::move(jObj);
    }

    // Ensure directory exists
    try
    {
        std::filesystem::path p(m_path);
        if (p.has_parent_path())
        {
            std::filesystem::create_directories(p.parent_path());
        }
    }
    catch (...)
    {
        // Non-fatal; try writing anyway
    }

    // Pretty print with 2-space indentation
    std::ofstream ofs(m_path, std::ios::binary);
    if (!ofs)
    {
        STAR_THROW(std::string("Failed to open JSON for writing: ") + m_path);
    }
    ofs << std::setw(2) << root;
    ofs.flush();
    if (!ofs)
    {
        STAR_THROW(std::string("Failed to write JSON to: ") + m_path);
    }
}

std::optional<SceneFile::LoadedObjectInfo> SceneFile::tryReadObjectInfo(const std::string &name)
{
    // Load file
    std::ifstream ifs(m_path, std::ios::binary);
    if (!ifs)
    {
        return std::nullopt; // not found or not readable
    }

    nlohmann::json root;
    try
    {
        ifs >> root;
    }
    catch (const std::exception &ex)
    {
        STAR_THROW(std::string("JSON parse error: ") + ex.what());
    }

    // Navigate to /Scene/objects/<name>
    if (!root.contains("Scene") || !root["Scene"].contains("objects"))
        return std::nullopt;

    const nlohmann::json &objects = root["Scene"]["objects"];
    if (!objects.contains(name))
        return std::nullopt;

    const nlohmann::json &jObj = objects[name];

    // Required fields: position[], scale[], rotation_deg[], rotation_order
    auto readVec3 = [](const nlohmann::json &j, const char *key, glm::vec3 &out) -> bool {
        if (!j.contains(key) || !j[key].is_array() || j[key].size() != 3)
            return false;
        out.x = j[key][0].get<float>();
        out.y = j[key][1].get<float>();
        out.z = j[key][2].get<float>();
        return true;
    };

    glm::vec3 position{}, scale{}, rotDeg{};
    if (!readVec3(jObj, "position", position))
        return std::nullopt;
    if (!readVec3(jObj, "scale", scale))
        return std::nullopt;
    if (!readVec3(jObj, "rotation_deg", rotDeg))
        return std::nullopt;

    LoadedObjectInfo info{
        .position = position,
        .scale = scale,
        .rotationsToApply = ConvertFromEulerToGlobalRotations(rotDeg),
    };

    return info;
}
} // namespace star::service::scene_loader