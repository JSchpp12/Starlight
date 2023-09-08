#pragma once 

namespace star {
class BasicDevice {
public:

protected: 
#ifdef NDEBUG 
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif    


private: 

};
}