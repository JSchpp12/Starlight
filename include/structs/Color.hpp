#pragma once

namespace star {
	class Color {
	public:
		Color() = default; 
		Color(unsigned char r, unsigned char g, unsigned char b, unsigned char a) : rr(255.0f / r), rg(255.0f / g), rb(255.0f / b), ra(255.0f / a) {};
		Color(float r, float g, float b, float a) : rr(r), rg(g), rb(b), ra(a) {};

		void setR(const float& r){
			if (r <= 1.0f)
				this->rr = unsigned char(255 * r);
			else
				this->rr = unsigned char(255); 
		}
		void setG(const float& g) {
			if (g <= 1.0f)
				this->rg = unsigned char(255 * g);
			else
				this->rg = unsigned char(255);
		}
		void setB(const float& b) {
			if (b <= 1.0f)
				this->rb = unsigned char(255 * b);
			else
				this->rb = unsigned char(255);
		}
		void setA(const float& a) {
			if (a <= 1.0f)
				this->ra = unsigned char(255 * a);
			else
				this->ra = unsigned char(255);
		}
		int r() const { return int(rr * 255); }
		int g() const { return int(rg * 255); }
		int b() const { return int(rb * 255); }
		int a() const { return int(ra * 255); }
		unsigned char raw_r() const { return unsigned char(255 * rr); }
		unsigned char raw_g() const { return unsigned char(255 * rg); }
		unsigned char raw_b() const { return unsigned char(255 * rb); }
		unsigned char raw_a() const { return unsigned char(255 * ra); }

	protected:
		float rr = 0.0f, rg = 0.0f, rb = 0.0f, ra = 0.0f;
	};
}