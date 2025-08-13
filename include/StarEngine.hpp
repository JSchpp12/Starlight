#pragma once

#include "StarApplication.hpp"
#include "StarScene.hpp"
#include "StarWindow.hpp"
#include "TransferWorker.hpp"
#include "Manager.hpp"

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
    std::unique_ptr<StarDevice> renderingDevice = nullptr;
    std::unique_ptr<StarApplication> application = nullptr; 
    std::shared_ptr<SwapChainRenderer> mainRenderer = nullptr;
    std::unique_ptr<TransferWorker> transferWorker = nullptr;

  private:
    const bool OVERRIDE_APPLY_SINGLE_THREAD_MODE = false;

    static std::unique_ptr<StarWindow> CreateStarWindow(); 

    static std::unique_ptr<StarDevice> CreateStarDevice(StarWindow &window); 

    static std::unique_ptr<job::Manager> CreateManager(); 
};
} // namespace star