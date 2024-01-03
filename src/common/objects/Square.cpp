#include "Square.hpp"

std::unique_ptr<star::Square> star::Square::New()
{
	return std::unique_ptr<star::Square>(new Square()); 
}

std::unordered_map<star::Shader_Stage, star::StarShader> star::Square::getShaders()
{
	std::unordered_map<star::Shader_Stage, StarShader> shaders;

	//load vertex shader
	std::string vertShaderPath = ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "/shaders/vertColor.vert";
	shaders.insert(std::pair<star::Shader_Stage, StarShader>(star::Shader_Stage::vertex, StarShader(vertShaderPath, Shader_Stage::vertex)));

	//load fragment shader
	std::string fragShaderPath = ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "/shaders/vertColor.frag";
	shaders.insert(std::pair<star::Shader_Stage, StarShader>(star::Shader_Stage::fragment, StarShader(fragShaderPath, Shader_Stage::fragment)));

	return shaders;
}

star::Square::Square()
{
	load(); 
}

void star::Square::load()
{
	std::unique_ptr<std::vector<star::Vertex>> verts = std::unique_ptr<std::vector<star::Vertex>>(new std::vector<star::Vertex>{
			star::Vertex{
				glm::vec3{-0.5f, 0.0f, -0.5f},	//position
				glm::vec3{0.0f, 1.0f, 0.0f},	//normal - posy
				glm::vec3{0.0f, 1.0f, 0.0f}		//color
			},
			star::Vertex{
				glm::vec3{0.5f, 0.0f, -0.5f},	//position
				glm::vec3{0.0f, 1.0f, 0.0f},	//normal - posy
				glm::vec3{0.0f, 1.0f, 0.0f}		//color
			},
			star::Vertex{
				glm::vec3{0.5f, 0.0f, 0.5f},	//position
				glm::vec3{0.0f, 1.0f, 0.0f},	//normal - posy
				glm::vec3{1.0f, 0.0f, 0.0f}		//color
			},
			star::Vertex{
				glm::vec3{-0.5f, 0.0f, 0.5f},	//position
				glm::vec3{0.0f, 1.0f, 0.0f},	//normal - posy
				glm::vec3{0.0f, 1.0f, 0.0f}		//color
			},
		});
	std::unique_ptr<std::vector<uint32_t>> inds = std::unique_ptr<std::vector<uint32_t>>(new std::vector<uint32_t>{
		0,3,2,0,2,1
	});

	std::unique_ptr<star::VertColorMaterial> material = std::unique_ptr<star::VertColorMaterial>(new star::VertColorMaterial());
	auto newMeshs = std::vector<std::unique_ptr<star::StarMesh>>();
	newMeshs.emplace_back(std::unique_ptr<star::StarMesh>(new star::StarMesh(*verts, *inds, std::move(material), false)));

	this->meshes = std::move(newMeshs); 
}

std::pair<std::unique_ptr<star::StarBuffer>, std::unique_ptr<star::StarBuffer>> star::Square::loadGeometryBuffers(StarDevice& device)
{
	return std::pair<std::unique_ptr<StarBuffer>, std::unique_ptr<StarBuffer>>();
}