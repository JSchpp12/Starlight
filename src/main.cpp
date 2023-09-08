
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "StarEngine.hpp"

int main(){
    std::cout << "Hello World! One Day I Will be a Rendering Engine! Maybe even used for some games!" << std::endl; 

    //test basic creation of needed vulkan structures
    auto test = star::StarEngine::Builder().build(); 
}