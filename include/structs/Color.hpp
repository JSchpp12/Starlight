#pragma once

namespace star {
	class Color {
	public:
		Color() = default; 
		Color(unsigned char r, unsigned char g, unsigned char b, unsigned char a) : rr(r), rg(g), rb(b), ra(a) {}; 
		Color(int r, int g, int b, int a) : rr(r), rg(g), rb(b), ra(a) {};

		int r() { return int(rr); }
		int g() { return int(rg); }
		int b() { return int(rb); }
		int a() { return int(ra); }
		unsigned char raw_r() { return rr; }
		unsigned char raw_g() { return rg; }
		unsigned char raw_b() { return rb; }
		unsigned char raw_a() { return ra; }

	protected:
		unsigned char rr, rg, rb, ra;
	};
}