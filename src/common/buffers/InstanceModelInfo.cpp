#include "InstanceModelInfo.hpp"

void star::InstanceModelInfo::write(StarBuffer& buffer)
{
	buffer.map(); 

	buffer.writeToBuffer(this->displayMatrixInfo.data()); 

	buffer.unmap(); 
}