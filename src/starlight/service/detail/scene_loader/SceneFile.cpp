#include "starlight/service/detail/scene_loader/SceneFile.hpp"

#include <iostream>
#include <pxr/base/gf/vec3d.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdGeom/xformCommonAPI.h>
#include <sstream>

namespace star::service::scene_loader
{
inline float Deg2Rad(float d)
{
    return d * float(M_PI / 180.0);
}

std::array<std::pair<star::Type::Axis, float>, 3> ConvertFromEulerToGlobalRotations(
    const pxr::GfVec3f &rDeg, const pxr::UsdGeomXformCommonAPI::RotationOrder &order)
{

    float ax = rDeg[0];
    float ay = rDeg[1];
    float az = rDeg[2];

    switch (order)
    {
    case (pxr::UsdGeomXformCommonAPI::RotationOrder::RotationOrderXYZ):
        return std::array<std::pair<star::Type::Axis, float>, 3>{
            std::make_pair(star::Type::Axis::z, az),
            std::make_pair(star::Type::Axis::y, ay),
            std::make_pair(star::Type::Axis::x, ax),
        };
        break;
    }

    return {};
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
    pxr::UsdStageRefPtr stage = pxr::UsdStage::CreateNew(m_path);

    for (const auto &ele : sceneObjects.getStorage())
    {
        pxr::UsdGeomXform xform;
        {
            auto oss = std::ostringstream();
            oss << "/Scene/objects/" << ele.first;
            xform = pxr::UsdGeomXform::Define(stage, pxr::SdfPath(oss.str()));
        }

        pxr::UsdGeomXformCommonAPI api(xform);
        {
            const auto pos = ele.second->getInstance().getPosition();
            api.SetTranslate(pxr::GfVec3f(pos.x, pos.y, pos.z));
        }
        {
            const auto rotation = ExtractRotationDegrees(ele.second);
            api.SetRotate(pxr::GfVec3f(rotation.x, rotation.y, rotation.z));
        }
        {
            const auto scale = ele.second->getInstance().getScale();
            api.SetScale(pxr::GfVec3f(scale.x, scale.y, scale.z));
        }
    }

    stage->GetRootLayer()->Save();
}

std::optional<SceneFile::LoadedObjectInfo> SceneFile::tryReadObjectInfo(const std::string &name)
{
    pxr::UsdStageRefPtr stage = pxr::UsdStage::Open(m_path);
    if (!stage)
    {
        return std::nullopt;
    }

    std::ostringstream oss;
    oss << "/Scene/objects/" << name;

    const pxr::SdfPath rootPath(oss.str());
    pxr::UsdPrim root = stage->GetPrimAtPath(rootPath);
    if (!root)
    {
        return std::nullopt;
    }

    pxr::UsdGeomXform xform(root);
    pxr::UsdGeomXformCommonAPI api(xform);

    pxr::GfVec3f rDeg, s, pivot;
    pxr::GfVec3d t;
    auto rotOrder = pxr::UsdGeomXformCommonAPI::RotationOrderXYZ;

    bool reset = false;
    if (api.GetXformVectors(&t, &rDeg, &s, &pivot, &rotOrder, pxr::UsdTimeCode::Default()))
    {
        LoadedObjectInfo info{.position = glm::vec3(t[0], t[1], t[2]),
                              .scale = glm::vec3(s[0], s[1], s[2]),
                              .rotationsToApply = ConvertFromEulerToGlobalRotations(rDeg, rotOrder)};

        return info;
    }

    return std::nullopt;
}
} // namespace star::service::scene_loader