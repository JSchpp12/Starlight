#include "StarScene.hpp"

#include "ManagerController_RenderResource_GlobalInfo.hpp"
#include "ManagerController_RenderResource_LightInfo.hpp"

star::StarScene::StarScene(const uint8_t &numFramesInFlight, std::shared_ptr<StarCamera> camera) : camera(camera)
{
    initBuffers(numFramesInFlight);
}

star::StarScene::StarScene(const uint8_t &numFramesInFlight, std::shared_ptr<star::StarCamera> externalCamera,
                           std::vector<star::Handle> globalInfoBuffers, std::vector<star::Handle> lightInfoBuffers)
    : camera(camera), globalInfoBuffers(globalInfoBuffers), lightInfoBuffers(lightInfoBuffers)
{
}

int star::StarScene::add(std::unique_ptr<star::Light> newLight)
{
    this->lightCounter++;

    int lightIndex = this->lightList.size();
    this->lightList.emplace_back(std::move(newLight));

    return lightIndex;
}

int star::StarScene::add(std::unique_ptr<StarObject> newObject)
{
    int objHandle = objCounter;
    this->objects.insert(std::pair<int, std::unique_ptr<StarObject>>(objCounter, std::move(newObject)));
    this->objCounter++;
    return objHandle;
}

std::vector<std::reference_wrapper<star::StarObject>> star::StarScene::getObjects()
{
    auto result = std::vector<std::reference_wrapper<StarObject>>();
    for (auto &obj : this->objects)
    {
        result.push_back(*obj.second);
    }
    return result;
}

void star::StarScene::initBuffers(const uint8_t &numFramesInFlight)
{
    this->lightInfoBuffers.resize(numFramesInFlight);
    this->globalInfoBuffers.resize(numFramesInFlight);

    for (int i = 0; i < numFramesInFlight; i++)
    {
        this->globalInfoBuffers[i] =
            (ManagerRenderResource::addRequest(std::make_unique<ManagerController::RenderResource::GlobalInfo>(
                static_cast<uint8_t>(i), *this->camera, this->lightCounter)));
        this->lightInfoBuffers[i] = ManagerRenderResource::addRequest(
            std::make_unique<ManagerController::RenderResource::LightInfo>(static_cast<uint8_t>(i), this->lightList));
    }
}