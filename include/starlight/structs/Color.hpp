#pragma once

#include <cassert>

namespace star {
	class Color {
	public:
		Color() = default; 
		Color(unsigned char r, unsigned char g, unsigned char b) 
			: rr(r / 255.0f), rg(g / 255.0f), rb(b / 255.0f), ra(1.0f) {
			assert(verify() && "Overflow of color channels!");
		};
		Color(unsigned char r, unsigned char g, unsigned char b, unsigned char a) 
			: rr(r / 255.0f), rg(g / 255.0f), rb(b / 255.0f), ra(a / 255.0f) {};
		Color(float r, float g, float b, float a) : rr(r), rg(g), rb(b), ra(a) {
			assert(verify() && "Overflow of color channels!");
		};

		void setR(const float& r){
			if (r <= 1.0f )
				this->rr = r;
			else
				this->rr = 1.0f;
		}
		void setG(const float& g) {
			if (g <= 1.0f)
				this->rg = g;
			else
				this->rg = 1.0;
		}
		void setB(const float& b) {
			if (b <= 1.0f)
				this->rb = b;
			else
				this->rb = 1.0;
		}
		void setA(const float& a) {
			if (a <= 1.0f)
				this->ra = a;
			else
				this->ra = 1.0;
		}
		float getR() const { return rr; }
		float getG() const { return rg; }
		float getB() const { return rb; }
		float getA() const { return ra; }
	protected:
		float rr = 0.0f, rg = 0.0f, rb = 0.0f, ra = 0.0f;

	private: 
		inline bool verify() const{
			return this->rr <= 1.0f && this->rg <= 1.0f && this->rb <= 1.0f && this->ra <= 1.0f;
		}
	};
}