#pragma once

namespace star {
	class Color {
	public:
		Color() = default; 
		Color(unsigned char r, unsigned char g, unsigned char b, unsigned char a) : rr(255.0f / r), rg(255.0f / g), rb(255.0f / b), ra(255.0f / a) {};
		Color(float r, float g, float b, float a) : rr(r), rg(g), rb(b), ra(a) {};

		void setR(const float& r){
			if (r <= 1.0f)
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
		float r() const { return rr; }
		float g() const { return rg; }
		float b() const { return rb; }
		float a() const { return ra; }
		unsigned char raw_r() const { return unsigned char(255 * rr); }
		unsigned char raw_g() const { return unsigned char(255 * rg); }
		unsigned char raw_b() const { return unsigned char(255 * rb); }
		unsigned char raw_a() const { return unsigned char(255 * ra); }

	protected:
		float rr = 0.0f, rg = 0.0f, rb = 0.0f, ra = 0.0f;
	};
}