#include "InstanceNormalInfo.hpp"

void star::InstanceNormalInfo::writeBufferData(StarBuffer& buffer)
{
	buffer.map(); 
	for (int i = 0; i < this->objectInstances.size(); i++)
	{
		this->objectInstances[i]->updateBufferData(buffer, 1);
	}
	buffer.unmap(); 
}
