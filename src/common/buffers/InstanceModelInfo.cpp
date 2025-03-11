#include "InstanceModelInfo.hpp"

void star::TransferRequest::InstanceModel::writeData(star::StarBuffer& buffer) const
{
	buffer.map();
	for (int i = 0; i < this->displayMatrixInfo.size(); ++i){
		glm::mat4 info = glm::mat4(this->displayMatrixInfo[i]);
		buffer.writeToBuffer(&info, sizeof(glm::mat4), i * sizeof(glm::mat4));
	}

	buffer.unmap();
}

std::unique_ptr<star::TransferRequest::Memory<star::StarBuffer::BufferCreationArgs>> star::InstanceModelInfo::createTransferRequest() const
{
	return std::make_unique<TransferRequest::InstanceModel>(this->objectInstances);
}