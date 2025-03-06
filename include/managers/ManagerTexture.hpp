#pragma once

#include "StarTexture.hpp"
#include "StarManager.hpp"
#include "Handle.hpp"
#include "TextureManagerRequest.hpp"
#include "SharedFence.hpp"
#include "TransferWorker.hpp"

#include <boost/atomic.hpp>

#include <optional>
#include <memory>
#include <map>

namespace star {
class ManagerTexture {
public:
	struct FinalizedTextureRequest {
		std::unique_ptr<TextureManagerRequest> request = nullptr;
		std::unique_ptr<StarTexture> texture = nullptr; 
		std::unique_ptr<SharedFence> workingFence;
		boost::atomic<bool> cpuWorkDoneByTransferThread = true; 

		FinalizedTextureRequest(std::unique_ptr<TextureManagerRequest> request) 
		: request(std::move(request)){}
	};



	//void update(); 

private:
	static TransferWorker* managerWorker; 
	static StarDevice* managerDevice; 
	static int bufferCounter;
	static int staticTextureIDCoutner;
	static int dynamicTextureIDCoutner;

	static std::vector<std::unique_ptr<FinalizedTextureRequest>> allTextures;
	static std::unordered_map<Handle, std::unique_ptr<FinalizedTextureRequest>*> updateableTextures;
	static std::unordered_map<Handle, std::unique_ptr<FinalizedTextureRequest>*> staticTextures;

};
}