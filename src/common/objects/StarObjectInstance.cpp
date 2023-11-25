#include "StarObjectInstance.hpp"

void star::StarObjectInstance::updateBufferData(StarBuffer& buffer, int bufferIndex)
{
	switch (bufferIndex) {
	case(0):
		auto displayMatrix = this->getDisplayMatrix();
		buffer.writeToIndex(&displayMatrix, this->instanceIndex); 
		break;
	case(1):
		auto normalMatrix = glm::inverseTranspose(getDisplayMatrix());
		buffer.writeToIndex(&normalMatrix, this->instanceIndex);
		break;
	}
}
