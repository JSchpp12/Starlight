#pragma once

#include "StarApplication.hpp"
#include "StarScene.hpp"
#include "StarWindow.hpp"
#include "TransferWorker.hpp"
#include "DeviceManager.hpp"

#include <memory>
#include <string>
#include <vector>

namespace star
{
class StarEngine
{
  public:
    StarEngine(std::unique_ptr<StarApplication> application);

    virtual ~StarEngine();

    void run();

  protected:
    std::unique_ptr<StarWindow> window = nullptr;
    core::DeviceManager deviceManager; 
    std::unique_ptr<StarApplication> application = nullptr; 
    std::shared_ptr<SwapChainRenderer> mainRenderer = nullptr;
    std::unique_ptr<TransferWorker> transferWorker = nullptr;

  private:
    const bool OVERRIDE_APPLY_SINGLE_THREAD_MODE = false;

    static std::unique_ptr<StarWindow> CreateStarWindow(); 

    static StarDevice CreateStarDevice(StarWindow &window); 

    static std::unique_ptr<job::TaskManager> CreateManager(); 
};
} // namespace star