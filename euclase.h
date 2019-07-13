#ifndef EUCLASE_H
#define EUCLASE_H

#include <QPoint>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace euclase {

template <typename T>
static inline T clamp(T a, T min, T max)
{
	return std::max(min, std::min(max, a));
}

static inline int gray(int r, int g, int b)
{
	return (306 * r + 601 * g + 117 * b) / 1024;
}

static inline float gamma(float v)
{
	return sqrt(v);
}

static inline float degamma(float v)
{
	return v * v;
}

struct PixelRGBA {
	uint8_t r, g, b, a;
	PixelRGBA()
		: r(0)
		, g(0)
		, b(0)
		, a(0)
	{
	}
	PixelRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
		: r(r)
		, g(g)
		, b(b)
		, a(a)
	{
	}
};

struct PixelGrayA {
	uint8_t l, a;
	PixelGrayA()
		: l(0)
		, a(0)
	{
	}
	PixelGrayA(uint8_t y, uint8_t a = 255)
		: l(y)
		, a(a)
	{
	}
};

class FPixelRGB {
public:
	float r;
	float g;
	float b;
	FPixelRGB()
		: r(0)
		, g(0)
		, b(0)
	{
	}
	FPixelRGB(float r, float g, float b)
		: r(r)
		, g(g)
		, b(b)
	{
	}
	FPixelRGB(PixelRGBA const &src)
		: r(float(src.r / 255.0))
		, g(float(src.g / 255.0))
		, b(float(src.b / 255.0))
	{
	}
	FPixelRGB operator + (FPixelRGB const &right) const
	{
		return FPixelRGB(r + right.r, g + right.g, b + right.b);
	}
	FPixelRGB operator * (float t) const
	{
		return FPixelRGB(r * t, g * t, b * t);
	}
	void operator += (FPixelRGB const &o)
	{
		r += o.r;
		g += o.g;
		b += o.b;
	}
	void add(FPixelRGB const &p, float v)
	{
		r += p.r * v;
		g += p.g * v;
		b += p.b * v;
	}
	void sub(FPixelRGB const &p, float v)
	{
		r -= p.r * v;
		g -= p.g * v;
		b -= p.b * v;
	}

	void operator *= (float t)
	{
		r *= t;
		g *= t;
		b *= t;
	}
	uint8_t r8() const
	{
		if (r <= 0) return 0;
		if (r >= 1) return 255;
		return (uint8_t)floor(r * 255 + 0.5);
	}
	uint8_t g8() const
	{
		if (g <= 0) return 0;
		if (g >= 1) return 255;
		return (uint8_t)floor(g * 255 + 0.5);
	}
	uint8_t b8() const
	{
		if (b <= 0) return 0;
		if (b >= 1) return 255;
		return (uint8_t)floor(b * 255 + 0.5);
	}
	PixelRGBA color(float amount) const
	{
		if (amount == 1) {
			return PixelRGBA(r8(), g8(), b8());
		} else if (amount == 0) {
			return PixelRGBA(0, 0, 0);
		}
		float m = 1 / amount;
		FPixelRGB p = *this * m;
		return PixelRGBA(p.r8(), p.g8(), p.b8());
	}
	operator PixelRGBA () const
	{
		return PixelRGBA(r8(), g8(), b8());
	}
};

class FPixelGray {
public:
	float l;
	FPixelGray()
		: l(0)
	{
	}
	FPixelGray(float y)
		: l(y)
	{
	}
	FPixelGray(PixelGrayA const &src)
		: l(src.l / 255.0)
	{
	}
	FPixelGray operator + (FPixelGray const &right) const
	{
		return FPixelGray(l + right.l);
	}
	FPixelGray operator * (float t) const
	{
		return FPixelGray(l * t);
	}
	void operator += (FPixelGray const &o)
	{
		l += o.l;
	}
	void add(FPixelGray const &p, float v)
	{
		v += p.l * v;
	}
	void sub(FPixelGray const &p, float v)
	{
		v -= p.l * v;
	}

	void operator *= (float t)
	{
		l *= t;
	}
	uint8_t y8() const
	{
		if (l <= 0) return 0;
		if (l >= 1) return 255;
		return (uint8_t)floor(l * 255 + 0.5);
	}
	PixelGrayA color(float amount) const
	{
		if (amount == 1) {
			return PixelGrayA(y8());
		} else if (amount == 0) {
			return PixelGrayA(0);
		}
		float m = 1 / amount;
		FPixelGray p = *this * m;
		return PixelGrayA(p.y8());
	}
	PixelGrayA toPixelGrayA() const
	{
		return PixelGrayA(y8());
	}
};

class FPixelRGBA {
public:
	float r;
	float g;
	float b;
	float a;
	FPixelRGBA()
		: r(0)
		, g(0)
		, b(0)
		, a(0)
	{
	}
	FPixelRGBA(float r, float g, float b, float a = 1)
		: r(r)
		, g(g)
		, b(b)
		, a(a)
	{
	}
	FPixelRGBA(PixelRGBA const &src)
		: r(float(src.r / 255.0))
		, g(float(src.g / 255.0))
		, b(float(src.b / 255.0))
		, a(float(src.a / 255.0))
	{
	}
	FPixelRGBA operator + (FPixelRGBA const &right) const
	{
		return FPixelRGBA(r + right.r, g + right.g, b + right.b);
	}
	FPixelRGBA operator * (float t) const
	{
		return FPixelRGBA(r * t, g * t, b * t);
	}
	void operator += (FPixelRGBA const &o)
	{
		r += o.r;
		g += o.g;
		b += o.b;
	}
	void operator *= (float t)
	{
		r *= t;
		g *= t;
		b *= t;
	}
	void add(FPixelRGBA const &p, float v)
	{
		v *= p.a;
		a += v;
		r += p.r * v;
		g += p.g * v;
		b += p.b * v;
	}
	void sub(FPixelRGBA const &p, float v)
	{
		v *= p.a;
		a -= v;
		r -= p.r * v;
		g -= p.g * v;
		b -= p.b * v;
	}
	uint8_t r8() const
	{
		if (r <= 0) return 0;
		if (r >= 1) return 255;
		return (uint8_t)floor(r * 255 + 0.5);
	}
	uint8_t g8() const
	{
		if (g <= 0) return 0;
		if (g >= 1) return 255;
		return (uint8_t)floor(g * 255 + 0.5);
	}
	uint8_t b8() const
	{
		if (b <= 0) return 0;
		if (b >= 1) return 255;
		return (uint8_t)floor(b * 255 + 0.5);
	}
	uint8_t a8() const
	{
		if (a <= 0) return 0;
		if (a >= 1) return 255;
		return (uint8_t)floor(a * 255 + 0.5);
	}
	PixelRGBA color(float amount) const
	{
		if (amount == 0) {
			return PixelRGBA(0, 0, 0, 0);
		}
		FPixelRGBA pixel(*this);
		pixel *= (1.0f / pixel.a);
		pixel.a = pixel.a / amount;
		pixel.a = clamp(pixel.a, 0.0f, 1.0f);
		return PixelRGBA(pixel.r8(), pixel.g8(), pixel.b8(), pixel.a8());
	}
	PixelRGBA toPixelRGBAa(float amount) const
	{
		return color(amount);
	}
	operator PixelRGBA () const
	{
		return PixelRGBA(r8(), g8(), b8(), a8());
	}
};

class FPixelGrayA {
public:
	float l;
	float a;
	FPixelGrayA()
		: l(0)
		, a(0)
	{
	}
	FPixelGrayA(float v, float a = 1)
		: l(v)
		, a(a)
	{
	}
	FPixelGrayA(PixelGrayA const &src)
		: l(float(src.l / 255.0))
		, a(float(src.a / 255.0))
	{
	}
	FPixelGrayA operator + (FPixelGrayA const &right) const
	{
		return FPixelGrayA(l + right.l);
	}
	FPixelGrayA operator * (float t) const
	{
		return FPixelGrayA(l * t);
	}
	void operator += (FPixelGrayA const &o)
	{
		l += o.l;
	}
	void operator *= (float t)
	{
		l *= t;
	}
	void add(FPixelGrayA const &p, float v)
	{
		v *= p.a;
		a += v;
		v += p.l * v;
	}
	void sub(FPixelGrayA const &p, float v)
	{
		v *= p.a;
		a -= v;
		v -= p.l * v;
	}
	uint8_t v8() const
	{
		if (l <= 0) return 0;
		if (l >= 1) return 255;
		return (uint8_t)floor(l * 255 + 0.5);
	}
	uint8_t a8() const
	{
		if (a <= 0) return 0;
		if (a >= 1) return 255;
		return (uint8_t)floor(a * 255 + 0.5);
	}
	PixelGrayA color(float amount) const
	{
		if (amount == 0) {
			return PixelGrayA(0, 0);
		}
		FPixelGrayA pixel(*this);
		pixel *= (1.0f / pixel.a);
		pixel.a = pixel.a / amount;
		pixel.a = clamp(pixel.a, 0.0f, 1.0f);
		return PixelGrayA(pixel.v8(), pixel.a8());
	}
	PixelGrayA toPixelGrayAa(float amount) const
	{
		return color(amount);
	}
};

static inline FPixelRGBA gamma(FPixelRGBA const &pix)
{
	return FPixelRGBA(gamma(pix.r), gamma(pix.g), gamma(pix.b), pix.a);
}

static inline FPixelRGBA degamma(FPixelRGBA const &pix)
{
	return FPixelRGBA(degamma(pix.r), degamma(pix.g), degamma(pix.b), pix.a);
}

// cubic bezier curve

double cubicBezierPoint(double p0, double p1, double p2, double p3, double t);
double cubicBezierGradient(double p0, double p1, double p2, double p3, double t);
QPointF cubicBezierPoint(QPointF &p0, QPointF &p1, QPointF &p2, QPointF &p3, double t);
void cubicBezierSplit(QPointF *p0, QPointF *p1, QPointF *p2, QPointF *p3, QPointF *q0, QPointF *q1, QPointF *q2, QPointF *q3, double t);

} // namespace euclase

#endif // EUCLASE_H
