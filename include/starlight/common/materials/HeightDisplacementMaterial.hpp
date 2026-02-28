//#pragma once
//
//#include "Color.hpp"
//#include "StarMaterial.hpp"
//#include "StarTexture.hpp"
//
//#include <memory>
//#include <vector>
//
//namespace star {
//	class HeightDisplacementMaterial : public StarMaterial {
//	public:
//		HeightDisplacementMaterial(int size_x, int size_y)
//			: size_x(size_x), size_y(size_y),
//			texture(std::make_unique<StarTexture>(StarTexture::TextureCreateSettings::createDefault(size_x, size_y))) {};
//
//		// Inherited via StarMaterial
//		void prep(StarDevice& device) override;
//
//		void applyDescriptorSetLayouts(star::StarDescriptorSetLayout::Builder& constBuilder) override;
//
//		StarTexture& getTexture() { return *this->texture; }
//
//	protected:
//		std::unique_ptr<StarTexture> texture;
//		int size_x = 0, size_y = 0;
//
//		// Inherited via StarMaterial
//		void cleanup(StarDevice& device) override;
//
//		void buildDescriptorSet(StarDevice& device, StarShaderInfo::Builder& builder, const int& frameInFlightIndex) override;
//	};
//}