#ifndef ALPHABLEND_H
#define ALPHABLEND_H

#include <stdint.h>

class AlphaBlend {
public:
	AlphaBlend();

	struct RGBA8888 {
		uint8_t r, g, b, a;
		RGBA8888(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0, uint8_t a = 255)
			: r(r)
			, g(g)
			, b(b)
			, a(a)
		{
		}
	};

	static inline int div255(int v)
	{
		return (v * 257 + 256) / 65536;
	}

	static inline RGBA8888 blend(RGBA8888 const &base, RGBA8888 const &over)
	{
		if (over.a == 0) return base;
		if (base.a == 0 || over.a == 255) return over;
		int r = over.r * over.a * 255 + base.r * base.a * (255 - over.a);
		int g = over.g * over.a * 255 + base.g * base.a * (255 - over.a);
		int b = over.b * over.a * 255 + base.b * base.a * (255 - over.a);
		int a = over.a * 255 + base.a * (255 - over.a);
		return RGBA8888(r / a, g / a, b / a, div255(a));
	}
};

#endif // ALPHABLEND_H
