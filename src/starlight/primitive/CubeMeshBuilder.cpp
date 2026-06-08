#include "starlight/primitive/CubeMeshBuilder.hpp"

namespace star::primitive
{
MeshData BuildCubeMesh(const std::vector<CubeDesc> &desc, std::shared_ptr<star::StarMaterial> material)
{
    constexpr float half = 0.5f;

    MeshData mesh;
    constexpr glm::vec3 color{1.0f, 0.0f, 0.0f};
    mesh.vertices = {
        // Front face: +Z
        {
            .pos = {-half, -half, half},
            .normal = {0.0f, 0.0f, 1.0f},
            .color = color,
            .texCoord = {0.0f, 1.0f},
        },
        {
            .pos = {half, -half, half},
            .normal = {0.0f, 0.0f, 1.0f},
            .color = color,
            .texCoord = {1.0f, 1.0f},
        },
        {
            .pos = {half, half, half},
            .normal = {0.0f, 0.0f, 1.0f},
            .color = color,
            .texCoord = {1.0f, 0.0f},
        },
        {
            .pos = {-half, half, half},
            .normal = {0.0f, 0.0f, 1.0f},
            .color = color,
            .texCoord = {0.0f, 0.0f},
        },

        // Back face: -Z
        {
            .pos = {half, -half, -half},
            .normal = {0.0f, 0.0f, -1.0f},
            .color = color,
            .texCoord = {0.0f, 1.0f},
        },
        {
            .pos = {-half, -half, -half},
            .normal = {0.0f, 0.0f, -1.0f},
            .color = color,
            .texCoord = {1.0f, 1.0f},
        },
        {
            .pos = {-half, half, -half},
            .normal = {0.0f, 0.0f, -1.0f},
            .color = color,
            .texCoord = {1.0f, 0.0f},
        },
        {
            .pos = {half, half, -half},
            .normal = {0.0f, 0.0f, -1.0f},
            .color = color,
            .texCoord = {0.0f, 0.0f},
        },

        // Left face: -X
        {
            .pos = {-half, -half, -half},
            .normal = {-1.0f, 0.0f, 0.0f},
            .color = color,
            .texCoord = {0.0f, 1.0f},
        },
        {
            .pos = {-half, -half, half},
            .normal = {-1.0f, 0.0f, 0.0f},
            .color = color,
            .texCoord = {1.0f, 1.0f},
        },
        {
            .pos = {-half, half, half},
            .normal = {-1.0f, 0.0f, 0.0f},
            .color = color,
            .texCoord = {1.0f, 0.0f},
        },
        {
            .pos = {-half, half, -half},
            .normal = {-1.0f, 0.0f, 0.0f},
            .color = color,
            .texCoord = {0.0f, 0.0f},
        },

        // Right face: +X
        {
            .pos = {half, -half, half},
            .normal = {1.0f, 0.0f, 0.0f},
            .color = color,
            .texCoord = {0.0f, 1.0f},
        },
        {
            .pos = {half, -half, -half},
            .normal = {1.0f, 0.0f, 0.0f},
            .color = color,
            .texCoord = {1.0f, 1.0f},
        },
        {
            .pos = {half, half, -half},
            .normal = {1.0f, 0.0f, 0.0f},
            .color = color,
            .texCoord = {1.0f, 0.0f},
        },
        {
            .pos = {half, half, half},
            .normal = {1.0f, 0.0f, 0.0f},
            .color = color,
            .texCoord = {0.0f, 0.0f},
        },

        // Top face: +Y
        {
            .pos = {-half, half, half},
            .normal = {0.0f, 1.0f, 0.0f},
            .color = color,
            .texCoord = {0.0f, 1.0f},
        },
        {
            .pos = {half, half, half},
            .normal = {0.0f, 1.0f, 0.0f},
            .color = color,
            .texCoord = {1.0f, 1.0f},
        },
        {
            .pos = {half, half, -half},
            .normal = {0.0f, 1.0f, 0.0f},
            .color = color,
            .texCoord = {1.0f, 0.0f},
        },
        {
            .pos = {-half, half, -half},
            .normal = {0.0f, 1.0f, 0.0f},
            .color = color,
            .texCoord = {0.0f, 0.0f},
        },

        // Bottom face: -Y
        {
            .pos = {-half, -half, -half},
            .normal = {0.0f, -1.0f, 0.0f},
            .color = color,
            .texCoord = {0.0f, 1.0f},
        },
        {
            .pos = {half, -half, -half},
            .normal = {0.0f, -1.0f, 0.0f},
            .color = color,
            .texCoord = {1.0f, 1.0f},
        },
        {
            .pos = {half, -half, half},
            .normal = {0.0f, -1.0f, 0.0f},
            .color = color,
            .texCoord = {1.0f, 0.0f},
        },
        {
            .pos = {-half, -half, half},
            .normal = {0.0f, -1.0f, 0.0f},
            .color = color,
            .texCoord = {0.0f, 0.0f},
        },
    };

    mesh.indices = {
        // Front
        0,
        1,
        2,
        2,
        3,
        0,

        // Back
        4,
        5,
        6,
        6,
        7,
        4,

        // Left
        8,
        9,
        10,
        10,
        11,
        8,

        // Right
        12,
        13,
        14,
        14,
        15,
        12,

        // Top
        16,
        17,
        18,
        18,
        19,
        16,

        // Bottom
        20,
        21,
        22,
        22,
        23,
        20,
    };

    return mesh;
}
} // namespace star::primitive