#pragma once

#include "BasicWindow.hpp"

#include <memory>

namespace star {
class StarEngine {
public:
	class Builder {
	public:
		Builder() {};
		std::unique_ptr<StarEngine> build() {
			return std::unique_ptr<StarEngine>(new StarEngine()); 
		}

	private:

	};

	virtual ~StarEngine();

protected:
	std::unique_ptr<BasicWindow> window;

	StarEngine();

private:

};
}