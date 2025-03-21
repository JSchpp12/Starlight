#include "Grid.hpp"
namespace star {
	Grid::~Grid(){}
	
	Grid::Grid(int vertX, int vertY) :StarObject(), vertX(vertX), vertY(vertY) {
		std::unique_ptr<std::vector<Vertex>> verts = std::make_unique<std::vector<Vertex>>(); 
		std::unique_ptr<std::vector<uint32_t>> indices = std::make_unique<std::vector<uint32_t>>(); 
		std::shared_ptr<VertColorMaterial> material = std::shared_ptr<VertColorMaterial>(new VertColorMaterial());

		this->loadGeometry(verts, indices); 
		// this->meshes.push_back(std::unique_ptr<StarMesh>(new StarMesh(*verts, *indices, material, false)));
	}

	Grid::Grid(int vertX, int vertY, std::shared_ptr<StarMaterial> material): vertX(vertX), vertY(vertY)
	{
		std::unique_ptr<std::vector<Vertex>> verts = std::make_unique<std::vector<Vertex>>();
		std::unique_ptr<std::vector<uint32_t>> indices = std::make_unique<std::vector<uint32_t>>();

		this->loadGeometry(verts, indices); 

		// this->meshes.push_back(std::unique_ptr<StarMesh>(new StarMesh(*verts, *indices, material, false)));
	}

	void Grid::loadGeometry(std::unique_ptr<std::vector<Vertex>>& verts, std::unique_ptr<std::vector<uint32_t>>& indices)
	{
		float stepSizeX = 1.0f / (vertX - 1);
		float stepSizeY = 1.0f / (vertY - 1);
		float xCounter = 0.0f;
		uint32_t indexCounter = 0;

		for (int i = 0; i < vertY; i++) {

			for (int j = 0; j < vertX; j++) {
				verts->push_back(Vertex{
					glm::vec3{stepSizeY * j, 0.0f, stepSizeX * i},
					glm::vec3{0.0f, 1.0f, 0.0f},
					glm::vec3{1.0f, 1.0f, 1.0f},
					glm::vec2{stepSizeY * i, stepSizeX * j}							//texture coordinate
					});

				if (j % 2 == 1 && i % 2 == 1) {
					//this is a 'central' vert where drawing should be based around
					// 
					//uppper left
					uint32_t center = indexCounter;
					uint32_t centerLeft = indexCounter - 1;
					uint32_t centerRight = indexCounter + 1;
					uint32_t upperLeft = indexCounter - 1 - vertX;
					uint32_t upperCenter = indexCounter - vertX;
					uint32_t upperRight = indexCounter - vertX + 1;
					uint32_t lowerLeft = indexCounter + vertX - 1;
					uint32_t lowerCenter = indexCounter + vertX;
					uint32_t lowerRight = indexCounter + vertX + 1;
					//1
					indices->push_back(center);
					indices->push_back(upperLeft);
					indices->push_back(centerLeft);
					//2
					indices->push_back(center);
					indices->push_back(upperCenter);
					indices->push_back(upperLeft);

					if (i != vertY - 1 && j == vertX - 1)
					{
						//side piece
						//cant do 3,4,5,6,
						//7
						indices->push_back(center);
						indices->push_back(lowerLeft);
						indices->push_back(lowerCenter);
						//8
						indices->push_back(center);
						indices->push_back(centerLeft);
						indices->push_back(lowerLeft);

					}
					else if (i == vertY - 1 && j != vertX - 1)
					{
						//bottom piece
						//cant do 5,6,7,8
						//3
						indices->push_back(center);
						indices->push_back(upperRight);
						indices->push_back(upperCenter);
						//4
						indices->push_back(center);
						indices->push_back(centerRight);
						indices->push_back(upperRight);
					}
					else if (i != vertY - 1 && j != vertX - 1) {
						//3
						indices->push_back(center);
						indices->push_back(upperRight);
						indices->push_back(upperCenter);
						//4
						indices->push_back(center);
						indices->push_back(centerRight);
						indices->push_back(upperRight);
						//5
						indices->push_back(center);
						indices->push_back(lowerRight);
						indices->push_back(centerRight);
						//6
						indices->push_back(center);
						indices->push_back(lowerCenter);
						indices->push_back(lowerRight);
						//7
						indices->push_back(center);
						indices->push_back(lowerLeft);
						indices->push_back(lowerCenter);
						//8
						indices->push_back(center);
						indices->push_back(centerLeft);
						indices->push_back(lowerLeft);
					}

				}
				indexCounter++;
			}
		}
	}

	std::unordered_map<star::Shader_Stage, StarShader> Grid::getShaders()
	{
		//default shader is 
		auto shaders = std::unordered_map<Shader_Stage, StarShader>(); 

		//load vertex shader
		std::string vertShaderPath = ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "/shaders/vertColor.vert";
		shaders.insert(std::pair<star::Shader_Stage, StarShader>(star::Shader_Stage::vertex, StarShader(vertShaderPath, Shader_Stage::vertex)));

		//load fragment shader
		std::string fragShaderPath = ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "/shaders/vertColor.frag";
		shaders.insert(std::pair<star::Shader_Stage, StarShader>(star::Shader_Stage::fragment, StarShader(fragShaderPath, Shader_Stage::fragment)));

		return shaders; 
	}
}