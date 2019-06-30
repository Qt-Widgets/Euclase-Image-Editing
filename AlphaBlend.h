#ifndef ALPHABLEND_H
#define ALPHABLEND_H

#include <stdint.h>
#include <math.h>

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

	struct YA88 {
		uint8_t y, g, b, a;
		YA88(uint8_t y = 0, uint8_t a = 255)
			: y(y)
			, a(a)
		{
		}
	};

	struct FloatRGBA {
		float r, g, b, a;
		FloatRGBA(float r = 0, float g = 0, float b = 0, float a = 1)
			: r(r)
			, g(g)
			, b(b)
			, a(a)
		{
		}
		FloatRGBA(RGBA8888 const &t)
			: r(float(t.r / 255.0))
			, g(float(t.g / 255.0))
			, b(float(t.b / 255.0))
			, a(float(t.a / 255.0))
		{
		}
		operator RGBA8888 () const
		{
			return RGBA8888((uint8_t)floor(r * 255 + 0.5), (uint8_t)floor(g * 255 + 0.5), (uint8_t)floor(b * 255 + 0.5), (uint8_t)floor(a * 255 + 0.5));
		}
	};

	static inline int gray(int r, int g, int b)
	{
		return (306 * r + 601 * g + 117 * b) / 1024;
	}

	static inline int div255(int v)
	{
		return (v * 257 + 256) / 65536;
	}

	class fixed_t {
	private:
		int16_t value;
		static int16_t lut_mul_[65536];
		static const int16_t *lut_mul();
	public:
		explicit fixed_t(int16_t v = 0)
			: value(v)
		{
		}
		explicit fixed_t(uint8_t v = 0)
			: value(v * (4096 * 514 + 100) / (65536 * 2))
		{
		}
		explicit fixed_t(float v = 0)
			: value((int16_t)floor(v * 4096 + 0.5))
		{
		}
		fixed_t operator + (fixed_t r) const
		{
			return fixed_t((int16_t)(value + r.value));
		}
		fixed_t operator - (fixed_t r) const
		{
			return fixed_t((int16_t)(value - r.value));
		}
		fixed_t operator * (fixed_t r) const
		{
			return fixed_t((int16_t)(((int32_t)value * r.value) >> 12));
		}
		fixed_t operator / (fixed_t r) const
		{
			return fixed_t((int16_t)((((int32_t)value << 12) + value) / r.value));
		}
		explicit operator uint8_t () const
		{
			if (value < 0) return 0;
			if (value > 0x0fff) return 255;
			return value >> 4;
		}
		explicit operator float () const
		{
			return float(value / 4096.0);
		}
		static fixed_t value0()
		{
			return fixed_t((int16_t)0);
		}
		static fixed_t value1()
		{
			return fixed_t((int16_t)4096);
		}
	};

	struct FixedRGBA {
		fixed_t r, g, b, a;
		FixedRGBA(float r = 0, float g = 0, float b = 0, float a = 1)
			: r(r)
			, g(g)
			, b(b)
			, a(a)
		{
		}
		FixedRGBA(RGBA8888 const &t)
			: r(t.r)
			, g(t.g)
			, b(t.b)
			, a(t.a)
		{
		}
		operator RGBA8888 () const
		{
			return RGBA8888((uint8_t)r, (uint8_t)g, (uint8_t)b, (uint8_t)a);
		}
	};

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

	static inline YA88 blend(YA88 const &base, YA88 const &over)
	{
		if (over.a == 0) return base;
		if (base.a == 0 || over.a == 255) return over;
		int y = over.y * over.a * 255 + base.y * base.a * (255 - over.a);
		int a = over.a * 255 + base.a * (255 - over.a);
		return YA88(y / a, div255(a));
	}

	static inline FloatRGBA blend(FloatRGBA const &base, FloatRGBA const &over)
	{
		if (over.a <= 0) return base;
		if (base.a <= 0 || over.a >= 1) return over;
		float r = over.r * over.a + base.r * base.a * (1 - over.a);
		float g = over.g * over.a + base.g * base.a * (1 - over.a);
		float b = over.b * over.a + base.b * base.a * (1 - over.a);
		float a = over.a + base.a * (1 - over.a);
		return FloatRGBA(r / a, g / a, b / a, a);
	}

	static inline RGBA8888 blend_with_gamma_collection(RGBA8888 const &base, RGBA8888 const &over)
	{
//		using T = FloatRGBA;
		using T = FixedRGBA;
		const float GAMMA = 2.0;
		auto Pre = [&](int t){
			float v = t / 255.0f;
			v = pow(v, GAMMA);
			return (float)v;
		};
		auto Post = [&](float v){
			v = pow(v, float(1 / GAMMA));
			return (uint8_t)floor(v * 255 + 0.5);
		};
		T fbase(Pre(base.r), Pre(base.g), Pre(base.b), float(base.a / 255.0));
		T fover(Pre(over.r), Pre(over.g), Pre(over.b), float(over.a / 255.0));
		T fblend = blend(fbase, fover);
		return RGBA8888(Post(float(fblend.r)), Post(float(fblend.g)), Post(float(fblend.b)), (uint8_t)floor((float)fblend.a * 255 + 0.5));
	}
};

#endif // ALPHABLEND_H
