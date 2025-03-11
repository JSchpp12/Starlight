#include "InstanceNormalInfo.hpp"


void star::TransferRequest::InstanceNormal::writeData(star::StarBuffer& buffer) const{
	buffer.map(); 

	std::vector<glm::mat4> inverseTranspose = std::vector<glm::mat4>(this->normalMatrixInfo.size());

	for (int i = 0; i < this->normalMatrixInfo.size(); i++){
		inverseTranspose[i] = glm::inverse(glm::transpose(this->normalMatrixInfo[i]));
	}
	buffer.writeToBuffer(inverseTranspose.data());
	buffer.unmap(); 
}

std::unique_ptr<star::TransferRequest::Memory<star::StarBuffer::BufferCreationArgs>> star::InstanceNormalInfo::createTransferRequest() const{
	return std::make_unique<star::TransferRequest::InstanceNormal>(this->objectInstances);
}
