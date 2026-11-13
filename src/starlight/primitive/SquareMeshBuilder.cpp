#include "starlight/primitive/SquareMeshBuilder.hpp"

namespace star::primitive
{
MeshData BuildSquareMesh(const SquareDesc &desc)
{
    const float halfW = desc.size.x * 0.5f;
    const float halfH = desc.size.y * 0.5f;

    MeshData mesh;

    const glm::vec3 color = glm::vec3(desc.color);

    mesh.vertices = {
        star::Vertex{
            .pos = {-halfW, -halfH, 0.0f}, .normal = {0.0f, 0.0f, 1.0f}, .color = color, .texCoord = {0.0f, 1.0f}},
        {.pos = {halfW, -halfH, 0.0f}, .normal = {0.0f, 0.0f, 1.0f}, .color = color, .texCoord = {1.0f, 1.0f}},
        {.pos = {halfW, halfH, 0.0f}, .normal = {0.0f, 0.0f, 1.0f}, .color = color, .texCoord = {1.0f, 0.0f}},
        {.pos = {-halfW, halfH, 0.0f}, .normal = {0.0f, 0.0f, 1.0f}, .color = color, .texCoord = {0.0f, 0.0f}}};

    mesh.indices = {0, 1, 2, 2, 3, 0};

    return mesh;
}
} // namespace star::primitive