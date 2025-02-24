#include "GlobalInfo.hpp"


void star::GlobalInfoTransfer::writeData(star::StarBuffer& buffer) const
{
    buffer.map(); 

    //update global ubo 
    GlobalUniformBufferObject globalUbo;
    globalUbo.proj = this->camera.getProjectionMatrix();
    //glm designed for openGL where the Y coordinate of the flip coordinates is inverted. Fix this by flipping the sign on the scaling factor of the Y axis in the projection matrix.
    globalUbo.proj[1][1] *= -1;
    globalUbo.view = this->camera.getViewMatrix();
    globalUbo.inverseView = glm::inverse(this->camera.getViewMatrix());
    globalUbo.numLights = static_cast<uint32_t>(this->numLights);

    buffer.writeToBuffer(&globalUbo, sizeof(globalUbo)); 

    buffer.unmap(); 
}

std::unique_ptr<star::BufferMemoryTransferRequest> star::GlobalInfo::createTransferRequest() const 
{
    return std::make_unique<GlobalInfoTransfer>(this->camera, this->numLights);
}
