#include "starlight/service/detail/scene_loader/SceneFile.hpp"

#include <pxr/base/tf/registryManager.h>
#include <pxr/base/gf/vec3d.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdGeom/xformCommonAPI.h>
#include <sstream>

namespace star::service::scene_loader
{

glm::vec3 ExtractRotationDegrees(const std::shared_ptr<StarObject> object)
{
    const auto r = glm::mat3(object->getInstance().getRotationMat());

    float sy = -r[2][0]; // -m20
    float cy = sqrtf(1.0f - sy * sy);

    float x, y, z;

    if (cy > 1e-6f)
    {
        // Standard Y-up Euler XYZ decomposition
        x = atan2f(r[2][1], r[2][2]); // rotX
        y = asinf(sy);                // rotY
        z = atan2f(r[1][0], r[0][0]); // rotZ
    }
    else
    {
        // Gimbal lock case
        x = atan2f(-r[1][2], r[1][1]);
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

SceneObjectTracker SceneFile::read()
{
    return SceneObjectTracker();
}
} // namespace star::service::scene_loader