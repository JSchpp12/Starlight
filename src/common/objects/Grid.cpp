//#include "Grid.hpp"
//namespace star {
//	Grid::Grid(int vertX, int vertY) : material(vertX, vertY), vertX(vertX), vertY(vertY) {
//		//calculate everything in x-y
//		std::vector<std::unique_ptr<StarMesh>> grid;
//		std::vector<std::vector<Color>> textureData = std::vector<std::vector<Color>>(vertY, std::vector<Color>(vertX));
//
//
//		auto verts = std::unique_ptr<std::vector<Vertex>>(new std::vector<Vertex>());
//		auto indices = std::unique_ptr<std::vector<uint32_t>>(new std::vector<uint32_t>());
//		bool test = true;
//		float stepSizeX = 1.0f / (vertX - 1);
//		float stepSizeY = 1.0f / (vertY - 1);
//		float xCounter = 0.0f;
//		uint32_t indexCounter = 0;
//
//		for (int i = 0; i < vertY; i++) {
//
//			for (int j = 0; j < vertX; j++) {
//				verts->push_back(Vertex{
//					glm::vec3{stepSizeY * j, 0.0f, stepSizeX * i},
//					glm::vec3{0.0f, 1.0f, 0.0f},
//					glm::vec3{0.0f,0.0f,0.0f},
//					glm::vec2{stepSizeY * i, stepSizeX * j}							//texture coordinate
//					});
//
//				material.getTexture().getRawData()->at(i).at(j) = Color{ 40,40,40,255 };
//
//				if (j % 2 == 1 && i % 2 == 1) {
//					//this is a 'central' vert where drawing should be based around
//					// 
//					//uppper left
//					uint32_t center = indexCounter;
//					uint32_t centerLeft = indexCounter - 1;
//					uint32_t centerRight = indexCounter + 1;
//					uint32_t upperLeft = indexCounter - 1 - vertX;
//					uint32_t upperCenter = indexCounter - vertX;
//					uint32_t upperRight = indexCounter - vertX + 1;
//					uint32_t lowerLeft = indexCounter + vertX - 1;
//					uint32_t lowerCenter = indexCounter + vertX;
//					uint32_t lowerRight = indexCounter + vertX + 1;
//					//1
//					indices->push_back(center);
//					indices->push_back(upperLeft);
//					indices->push_back(centerLeft);
//					//2
//					indices->push_back(center);
//					indices->push_back(upperCenter);
//					indices->push_back(upperLeft);
//
//					if (i != vertY - 1 && j == vertX - 1)
//					{
//						//side piece
//						//cant do 3,4,5,6,
//						//7
//						indices->push_back(center);
//						indices->push_back(lowerLeft);
//						indices->push_back(lowerCenter);
//						//8
//						indices->push_back(center);
//						indices->push_back(centerLeft);
//						indices->push_back(lowerLeft);
//
//					}
//					else if (i == vertY - 1 && j != vertX - 1)
//					{
//						//bottom piece
//						//cant do 5,6,7,8
//						//3
//						indices->push_back(center);
//						indices->push_back(upperRight);
//						indices->push_back(upperCenter);
//						//4
//						indices->push_back(center);
//						indices->push_back(centerRight);
//						indices->push_back(upperRight);
//					}
//					else if (i != vertY - 1 && j != vertX - 1) {
//						//3
//						indices->push_back(center);
//						indices->push_back(upperRight);
//						indices->push_back(upperCenter);
//						//4
//						indices->push_back(center);
//						indices->push_back(centerRight);
//						indices->push_back(upperRight);
//						//5
//						indices->push_back(center);
//						indices->push_back(lowerRight);
//						indices->push_back(centerRight);
//						//6
//						indices->push_back(center);
//						indices->push_back(lowerCenter);
//						indices->push_back(lowerRight);
//						//7
//						indices->push_back(center);
//						indices->push_back(lowerLeft);
//						indices->push_back(lowerCenter);
//						//8
//						indices->push_back(center);
//						indices->push_back(centerLeft);
//						indices->push_back(lowerLeft);
//					}
//
//				}
//				indexCounter++;
//			}
//		}
//
//		this->meshes.push_back(std::unique_ptr<StarMesh>(new StarMesh(std::move(verts), std::move(indices), material)));
//	}
//
//	void Grid::updateTexture(std::vector<int> locsX, std::vector<int> locsY, const std::vector<Color> newColor) {
//		assert(this->meshes.size() > 0 && "Make sure this function is only called after the prepRender phase");
//
//		RuntimeUpdateTexture& tex = this->material.getTexture();
//		for (int i = 0; i < locsX.size(); i++) {
//			tex.getRawData()->at(locsX[i]).at(locsY[i]) = newColor[i];
//		}
//
//		tex.updateGPU();
//	}
//
//	std::optional<glm::vec3> Grid::getWorldCoordsWhereRayIntersectsMe(glm::vec3 tail, glm::vec3 head)
//	{
//		glm::vec3 vectorDirection = glm::normalize((head - tail));
//		glm::vec3 planeNorm = glm::vec3(this->upVector);
//
//		//check for parallel vector plane
//		float denm = glm::dot(planeNorm, vectorDirection);
//		if (glm::abs(denm) > 0.0001f) {
//			auto dis = this->getCenter() - tail;
//			auto dot = glm::dot(glm::vec4(dis, 0.0), this->upVector);
//			float t = dot / denm;
//			if (t >= 0) {
//				auto point = tail + t * vectorDirection;
//				return std::optional<glm::vec3>(point);
//			}
//		}
//
//		return std::optional<glm::vec3>();
//	}
//
//	std::optional<glm::vec2> Grid::getXYCoordsWhereRayIntersectsMe(glm::vec3 tail, glm::vec3 head)
//	{
//		glm::vec3 modelLoc;
//
//		auto worldLoc = getWorldCoordsWhereRayIntersectsMe(tail, head);
//		if (!worldLoc.has_value())
//			return std::optional<glm::vec2>();
//		else
//			modelLoc = glm::inverse(this->getDisplayMatrix()) * glm::vec4(worldLoc.value(), 1.0);
//
//		//calculate step sizes 
//		float stepX = 1.0f / this->vertX;
//		float stepY = 1.0f / this->vertY;
//
//		int numXSteps = glm::floor(modelLoc.x / stepX);
//		int numYSteps = glm::floor(modelLoc.z / stepY);
//
//		return std::optional<glm::vec2>(glm::vec2(numXSteps, numYSteps));
//	}
//
//	glm::vec3 Grid::getCenter() {
//		glm::vec3 position = this->getPosition();
//		//position begins at corner, move to center
//		position.x = position.x + 0.5;
//		position.z = position.y + 0.5;
//
//		//scale it
//		glm::vec3 scaledPosition = position * this->getScale();
//
//		return scaledPosition;
//	}
//	std::unique_ptr<StarPipeline> Grid::buildPipeline(StarDevice& device, vk::Extent2D swapChainExtent, vk::PipelineLayout pipelineLayout, vk::RenderPass renderPass)
//	{
//		return std::unique_ptr<StarPipeline>();
//	}
//	std::unordered_map<star::Shader_Stage, StarShader> Grid::getShaders()
//	{
//		return std::unordered_map<star::Shader_Stage, StarShader>();
//	}
//}