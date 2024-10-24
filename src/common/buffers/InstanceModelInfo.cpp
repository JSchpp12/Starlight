#include "InstanceModelInfo.hpp"

void star::InstanceModelInfo::writeBufferData(StarBuffer& buffer)
{
	buffer.map(); 

	for (int i = 0; i < this->objectInstances.size(); i++) {
		this->objectInstances[i]->updateBufferData(buffer, 0); 
	}

	buffer.unmap(); 
}