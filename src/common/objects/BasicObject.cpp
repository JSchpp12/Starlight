#include "BasicObject.hpp"

#include "ManagerController_RenderResource_TextureFile.hpp"
#include "TransferRequest_IndicesInfo.hpp"
#include "TransferRequest_VertInfo.hpp"

#include "BumpMaterial.hpp"
#include "CastHelpers.hpp"
#include "FileHelpers.hpp"
#include "StarMesh.hpp"

#include "StarGraphicsPipeline.hpp"
#include "VertColorMaterial.hpp"

#include <boost/filesystem.hpp>

std::unordered_map<star::Shader_Stage, star::StarShader> star::BasicObject::getShaders()
{
    std::unordered_map<star::Shader_Stage, StarShader> shaders;

    bool isBumpyMaterial = false;
    bool isTextureMaterial = false;
    getTypeOfMaterials(isTextureMaterial, isBumpyMaterial);

    if (isBumpyMaterial)
    {
        // load vertex shader
        std::string vertShaderPath =
            ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "/shaders/bump.vert";
        shaders.insert(std::pair<star::Shader_Stage, StarShader>(star::Shader_Stage::vertex,
                                                                 StarShader(vertShaderPath, Shader_Stage::vertex)));

        // load fragment shader
        std::string fragShaderPath =
            ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "/shaders/bump.frag";
        shaders.insert(std::pair<star::Shader_Stage, StarShader>(star::Shader_Stage::fragment,
                                                                 StarShader(fragShaderPath, Shader_Stage::fragment)));
    }
    else if (isTextureMaterial)
    {
        // load vertex shader
        std::string vertShaderPath =
            ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "/shaders/default.vert";
        shaders.insert(std::pair<star::Shader_Stage, StarShader>(star::Shader_Stage::vertex,
                                                                 StarShader(vertShaderPath, Shader_Stage::vertex)));

        // load fragment shader
        std::string fragShaderPath =
            ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "/shaders/default.frag";
        shaders.insert(std::pair<star::Shader_Stage, StarShader>(star::Shader_Stage::fragment,
                                                                 StarShader(fragShaderPath, Shader_Stage::fragment)));
    }
    else
    {
        // load vertex shader
        std::string vertShaderPath =
            ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "/shaders/vertColor.vert";
        shaders.insert(std::pair<star::Shader_Stage, StarShader>(star::Shader_Stage::vertex,
                                                                 StarShader(vertShaderPath, Shader_Stage::vertex)));

        // load fragment shader
        std::string fragShaderPath =
            ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "/shaders/vertColor.frag";
        shaders.insert(std::pair<star::Shader_Stage, StarShader>(star::Shader_Stage::fragment,
                                                                 StarShader(fragShaderPath, Shader_Stage::fragment)));
    }

    return shaders;
}

std::vector<std::unique_ptr<star::StarMesh>> star::BasicObject::loadMeshes(core::device::DeviceContext &context)
{
    auto parent = file_helpers::GetParentDirectory(m_objFilePath).value().string();

    std::cout << "Loading object file: " << m_objFilePath << std::endl;

    /* Load Object From File */
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, m_objFilePath.c_str(), parent.c_str(), true))
    {
        throw std::runtime_error(warn + err);
    }
    if (warn != "")
    {
        std::cout << "An error occurred while loading obj file" << std::endl;
        std::cout << warn << std::endl;
        std::cout << "Loading will continue..." << std::endl;
    }

    size_t shapeCounter = 0;
    Handle loadMaterialTexture;
    std::unique_ptr<std::vector<Vertex>> verticies;
    std::unique_ptr<std::vector<uint32_t>> indicies;
    std::unique_ptr<std::vector<std::pair<unsigned int, unsigned int>>> sortedIds;
    std::vector<std::unique_ptr<StarMesh>> meshes(shapes.size());
    tinyobj::material_t *currMaterial = nullptr;
    std::unique_ptr<StarMaterial> objectMaterial;
    std::vector<std::shared_ptr<StarMaterial>> preparedMaterials;

    if (materials.size() != meshes.size() || materials.size() != m_meshMaterials.size())
    {
        throw std::runtime_error("All properties should match with the number of meshes contained in obj file");
    }

    // need to scale object so that it fits on screen
    // combine all attributes into a single object
    int dIndex = 0;
    for (const auto &shape : shapes)
    {
        // tinyobj ensures three verticies per triangle  -- assuming unique vertices
        const std::vector<tinyobj::index_t> &indicies = shape.mesh.indices;
        auto fullInd = std::vector<uint32_t>(shape.mesh.indices.size());
        auto vertices = std::vector<Vertex>(shape.mesh.indices.size());
        size_t vertCounter = 0;
        for (size_t faceIndex = 0; faceIndex < shape.mesh.material_ids.size(); faceIndex++)
        {
            for (int i = 0; i < 3; i++)
            {
                dIndex = (3 * faceIndex) + i;
                auto newVertex = Vertex();
                newVertex.pos = glm::vec3{attrib.vertices[3 * indicies[dIndex].vertex_index + 0],
                                          attrib.vertices[3 * indicies[dIndex].vertex_index + 1],
                                          attrib.vertices[3 * indicies[dIndex].vertex_index + 2]};
                newVertex.color = glm::vec3{
                    attrib.colors[3 * indicies[dIndex].vertex_index + 0],
                    attrib.colors[3 * indicies[dIndex].vertex_index + 1],
                    attrib.colors[3 * indicies[dIndex].vertex_index + 2],
                };

                if (attrib.normals.size() > 0)
                {
                    newVertex.normal = {
                        attrib.normals[3 * indicies[dIndex].normal_index + 0],
                        attrib.normals[3 * indicies[dIndex].normal_index + 1],
                        attrib.normals[3 * indicies[dIndex].normal_index + 2],
                    };
                }

                newVertex.texCoord = {attrib.texcoords[2 * indicies[dIndex].texcoord_index + 0],
                                      1.0f - attrib.texcoords[2 * indicies[dIndex].texcoord_index + 1]};

                vertices.at(vertCounter) = newVertex;
                fullInd.at(vertCounter) = star::CastHelpers::size_t_to_unsigned_int(vertCounter);
                vertCounter++;
            }
        }

        if (shape.mesh.material_ids.at(shapeCounter) != -1)
        {
            const auto meshVertSemaphore =
                context.getSemaphoreManager().submit(core::device::manager::SemaphoreRequest(false));
            const Handle meshVertBuffer = ManagerRenderResource::addRequest(
                context.getDeviceID(), context.getSemaphoreManager().get(meshVertSemaphore)->semaphore,
                std::make_unique<TransferRequest::VertInfo>(
                    context.getDevice().getDefaultQueue(Queue_Type::Tgraphics).getParentQueueFamilyIndex(),
                    vertices));

            const auto indSemaphore =
                context.getSemaphoreManager().submit(core::device::manager::SemaphoreRequest(false));

            const Handle meshIndBuffer = ManagerRenderResource::addRequest(
                context.getDeviceID(), context.getSemaphoreManager().get(indSemaphore)->semaphore,
                std::make_unique<TransferRequest::IndicesInfo>(
                    context.getDevice().getDefaultQueue(Queue_Type::Tgraphics).getParentQueueFamilyIndex(),
                    fullInd));

            // apply material from files to mesh -- will ignore passed values
            meshes.at(shapeCounter) =
                std::unique_ptr<StarMesh>(new StarMesh(meshVertBuffer, meshIndBuffer, vertices, fullInd,
                                                       m_meshMaterials.at(shape.mesh.material_ids[0]), false));
        }
        shapeCounter++;
    }

    return meshes;
}

std::vector<std::shared_ptr<star::StarMaterial>> star::BasicObject::LoadMaterials(const std::string &filePath)
{
    auto parentDirectory = file_helpers::GetParentDirectory(filePath).value().string();

    if (!star::file_helpers::FileExists(filePath))
    {
        throw std::runtime_error("Provided object file does not exist");
    }

    /* Load Object From File */
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> fileMaterials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &fileMaterials, &warn, &err, filePath.c_str(), parentDirectory.c_str(),
                          true))
    {
        throw std::runtime_error(warn + err);
    }
    if (warn != "")
    {
        std::cout << "An error occurred while loading obj file" << std::endl;
        std::cout << warn << std::endl;
        std::cout << "Loading will continue..." << std::endl;
    }

    std::vector<std::shared_ptr<StarMaterial>> materials;

    for (auto &fMaterial : fileMaterials)
    {
        const bool isTextureMaterial = !fMaterial.diffuse_texname.empty();
        const bool isBumpMaterial = isTextureMaterial && !fMaterial.bump_texname.empty();

        if (fMaterial.ambient[0] == 0)
        {
            fMaterial.ambient[0] = 1.0;
            fMaterial.ambient[1] = 1.0;
            fMaterial.ambient[2] = 1.0;
        }

        if (isBumpMaterial)
        {
            auto path = parentDirectory / boost::filesystem::path(fMaterial.bump_texname);
            materials.emplace_back(std::make_shared<star::BumpMaterial>(
                path.string(), fMaterial.diffuse_texname, glm::vec4(1.0), glm::vec4(1.0), glm::vec4(1.0),
                glm::vec4{fMaterial.diffuse[0], fMaterial.diffuse[1], fMaterial.diffuse[2], 1.0f},
                glm::vec4{fMaterial.specular[0], fMaterial.specular[1], fMaterial.specular[2], 1.0f},
                fMaterial.shininess));
        }
        else if (isTextureMaterial)
        {
            auto path = parentDirectory / boost::filesystem::path(fMaterial.diffuse_texname);
            materials.emplace_back(std::make_shared<star::TextureMaterial>(
                path.string(), glm::vec4(1.0), glm::vec4(1.0), glm::vec4(1.0),
                glm::vec4{fMaterial.diffuse[0], fMaterial.diffuse[1], fMaterial.diffuse[2], 1.0f},
                glm::vec4{fMaterial.specular[0], fMaterial.specular[1], fMaterial.specular[2], 1.0f},
                fMaterial.shininess));
        }
        else
        {
            // fall back to vert color material
            materials.emplace_back(std::make_shared<star::VertColorMaterial>(
                glm::vec4(1.0), glm::vec4(1.0), glm::vec4(1.0),
                glm::vec4{fMaterial.diffuse[0], fMaterial.diffuse[1], fMaterial.diffuse[2], 1.0f},
                glm::vec4{fMaterial.specular[0], fMaterial.specular[1], fMaterial.specular[2], 1.0f},
                fMaterial.shininess));
        }
    }

    return materials;
}

void star::BasicObject::getTypeOfMaterials(bool &isTextureMaterial, bool &isBumpMaterial) const
{
    assert(m_meshMaterials.size() > 0 && "Mesh materials should always exist");

    if (auto bumpDerived = std::dynamic_pointer_cast<BumpMaterial>(m_meshMaterials.front()))
    {
        isBumpMaterial = true;
    }
    else if (auto textureDerived = std::dynamic_pointer_cast<TextureMaterial>(m_meshMaterials.front()))
    {
        isTextureMaterial = true;
    }
}