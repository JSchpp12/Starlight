#include "ManagerTexture.hpp"

size_t star::ManagerTexture::counter = size_t(0); 
std::vector<star::StarTexture> star::ManagerTexture::textures = std::vector<std::unique_ptr<star::StarTexture>>(); 
std::stack<std::unique_ptr<star::ManagerTexture::Request>> star::ManagerTexture::newRequests = std::vector<std::unique_ptr<star::ManagerTexture::Request>>(); 

star::Handle star::ManagerTexture::submitRequest(std::unique_ptr<Request> newRequest)
{
	ManagerTexture::newRequests.push(std::move(newRequests)); 
	return Handle(++counter);
}
