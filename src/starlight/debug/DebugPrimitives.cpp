#include "starlight/debug/DebugPrimitives.hpp"

#include "starlight/primitive/CubeObject.hpp"
#include "starlight/primitive/SquareDesc.hpp"
#include "starlight/primitive/SquareObject.hpp"

namespace star::debug
{
std::unique_ptr<StarObject> CreateSquare(const SquareCreateInfo &info, ShaderResolver &shaderResolver)
{
    auto square = std::make_unique<star::primitive::SquareObject>(
        star::primitive::SquareDesc{.size = info.size, .color = info.color}, shaderResolver);

    square->isVisible = info.visible;
    square->drawNormals = info.drawNormals;
    square->drawBoundingBox = info.drawBoundingBox;

    return square;
}

std::shared_ptr<StarObject> CreateCube(std::vector<primitive::CubeDesc> info, ShaderResolver &shaderResolver)
{
    return std::make_shared<star::primitive::CubeObject>(std::move(info), shaderResolver);
}
} // namespace star::debug
