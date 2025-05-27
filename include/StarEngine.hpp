#pragma once

#include "StarApplication.hpp"
#include "StarBuffer.hpp"
#include "StarScene.hpp"
#include "StarWindow.hpp"
#include "TransferWorker.hpp"


#include <memory>
#include <string>
#include <vector>


namespace star
{
class StarEngine
{
  public:
    static void takeScreenshot(const std::string &path)
    {
        screenshotPath = std::make_unique<std::string>(path);
    }

    StarEngine();

    virtual ~StarEngine();

    void run();

    void init(StarApplication &application);

    StarScene &getScene()
    {
        return *this->currentScene;
    }

  protected:
    std::unique_ptr<StarWindow> window;
    std::unique_ptr<StarDevice> renderingDevice;
    std::unique_ptr<StarScene> currentScene;
    std::unique_ptr<SwapChainRenderer> mainRenderer;
    std::unique_ptr<TransferWorker> transferWorker;

    std::unique_ptr<StarBuffer> vertexBuffer, indexBuffer;
    std::vector<StarRenderGroup> renderGroups;

  private:
    static std::unique_ptr<std::string> screenshotPath;

    const bool OVERRIDE_APPLY_SINGLE_THREAD_MODE = false;
};
} // namespace star