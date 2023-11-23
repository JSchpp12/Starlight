//#pragma once
//
//#include "FileHelpers.hpp"
//#include "Enums.hpp"
//#include "StarResourceContainer.hpp"
//#include "ConfigFile.hpp"
//#include "FileResourceManager.hpp"
//#include "StarShader.hpp"
//
//#include "Compiler.hpp"
//
//#include <map> 
//#include <string> 
//#include <memory>
//
//namespace star {
//    class ShaderManager : public FileResourceManager<StarShader> {
//    public:
//        /// <summary>
//        /// Create shader manager with default shaders to be used if objects are not explicitly provided shaders
//        /// </summary>
//        ShaderManager() {};
//
//        void setDefault(const std::string& defaultVert, const std::string& defaultFrag);
//
//        ~ShaderManager();
//
//        StarShader& resource(const Handle& resourceHandle) override;
//    protected:
//
//        Handle createAppropriateHandle() override;
//        Handle_Type handleType() override { return Handle_Type::shader; }
//
//
//    private:
//        Handle defaultVertShader, defaultFragShader;
//
//    };
//}