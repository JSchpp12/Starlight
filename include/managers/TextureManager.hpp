#pragma once 
#include "FileResourceManager.hpp"
#include "Texture.hpp"
#include "Handle.hpp"

#include <string> 
#include <memory> 

namespace star {
    class TextureManager : private FileResourceManager<Texture> {
    public:
        TextureManager(const std::string& path) { this->addResource(path); }

        virtual ~TextureManager() { };

        virtual Handle addResource(const std::string& path) { return this->FileResourceManager<Texture>::addResource(path, std::make_unique<Texture>(path)); }
        virtual Texture& resource(const Handle& resourceHandle) override { return this->FileResourceManager<Texture>::resource(resourceHandle); }

    protected:
        virtual Handle createAppropriateHandle() override;
        virtual Handle_Type handleType() override { return Handle_Type::texture; }

    private:

    };
}