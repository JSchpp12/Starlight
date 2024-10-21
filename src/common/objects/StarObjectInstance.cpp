#include "StarObjectInstance.hpp"

void star::StarObjectInstance::updateBufferData(StarBuffer& buffer, int bufferIndex) const
{
	switch (bufferIndex) {
	case(0):
		auto displayMatrix = this->getDisplayMatrix();
		buffer.writeToBuffer(&displayMatrix, sizeof(displayMatrix), sizeof(displayMatrix) * this->instanceIndex);
		break;
	case(1):
		auto normalMatrix = glm::inverseTranspose(getDisplayMatrix());
		//buffer.writeToIndex(&normalMatrix, this->instanceIndex);
		buffer.writeToBuffer(&displayMatrix, sizeof(normalMatrix), sizeof(normalMatrix) * this->instanceIndex); 
		break;
	}
}
