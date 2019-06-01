
#include "median.h"
#ifdef _WIN32
#define _USE_MATH_DEFINES
#endif
#include <math.h>
#include <vector>
#include <string.h>
#include <stdint.h>

namespace {

inline uint8_t gray(uint8_t r, uint8_t g, uint8_t b)
{
	return (306 * r + 601 * g + 117 * b + 512) / 1024;
}

struct PixelRGBA;
struct PixelGrayA;

//

struct PixelRGBA {
	uint8_t r, g, b, a;
	PixelRGBA()
		: r(0)
		, g(0)
		, b(0)
		, a(0)
	{
	}
	PixelRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
		: r(r)
		, g(g)
		, b(b)
		, a(a)
	{
	}
	PixelRGBA(PixelGrayA const &pixel);
};

struct PixelGrayA {
	uint8_t y, a;
	PixelGrayA()
		: y(0)
		, a(0)
	{
	}
	PixelGrayA(uint8_t y, uint8_t a)
		: y(y)
		, a(a)
	{
	}
	PixelGrayA(PixelRGBA const &pixel);
};

inline uint8_t gray(PixelRGBA const &rgba)
{
	return rgba.a == 0 ? 0 : gray(rgba.r, rgba.g, rgba.b);
}

inline PixelRGBA::PixelRGBA(PixelGrayA const &pixel)
	: r(pixel.y)
	, g(pixel.y)
	, b(pixel.y)
	, a(pixel.a)
{
}

inline PixelGrayA::PixelGrayA(PixelRGBA const &pixel)
	: y(gray(pixel))
	, a(pixel.a)
{
}

//

struct FPixelRGBA {
	double r, g, b, a, n;
	FPixelRGBA()
		: r(0)
		, g(0)
		, b(0)
		, a(0)
		, n(0)
	{
	}
	FPixelRGBA(double r, double g, double b, double a)
		: r(r)
		, g(g)
		, b(b)
		, a(a)
		, n(1)
	{
	}
	void add(PixelRGBA const &t)
	{
		r += t.r * t.a;
		g += t.g * t.a;
		b += t.b * t.a;
		a += t.a;
		n++;
	}
	void sub(PixelRGBA const &t)
	{
		r -= t.r * t.a;
		g -= t.g * t.a;
		b -= t.b * t.a;
		a -= t.a;
		n--;
	}
	operator PixelRGBA ()
	{
		PixelRGBA pixel;
		pixel.r = (uint8_t)(r / a);
		pixel.g = (uint8_t)(g / a);
		pixel.b = (uint8_t)(b / a);
		pixel.a = (uint8_t)(a / n);
		return pixel;
	}
};

//

struct FPixelGray {
	double y, a, n;
	FPixelGray()
		: y(0)
		, a(0)
		, n(0)
	{
	}
	FPixelGray(double y, double a)
		: y(y)
		, a(a)
		, n(1)
	{
	}
	void add(PixelGrayA const &t)
	{
		y += t.y * t.a;
		a += t.a;
		n++;
	}
	void sub(PixelGrayA const &t)
	{
		y -= t.y * t.a;
		a -= t.a;
		n--;
	}
	operator PixelGrayA ()
	{
		PixelGrayA pixel;
		pixel.y = (uint8_t)(y / a);
		pixel.a = (uint8_t)(a / n);
		return pixel;
	}
};

//

class median_t {
private:
	int map256_[256];
	int map16_[16];
public:
	median_t()
	{
		clear();
	}
	void clear()
	{
		for (int i = 0; i < 256; i++) {
			map256_[i] = 0;
		}
		for (int i = 0; i < 16; i++) {
			map16_[i] = 0;
		}
	}
	void insert(uint8_t n)
	{
		map256_[n]++;
		map16_[n >> 4]++;
	}
	void remove(uint8_t n)
	{
		map256_[n]--;
		map16_[n >> 4]--;
	}
	uint8_t get()
	{
		int left, right;
		int lower, upper;
		lower = 0;
		upper = 0;
		left = 0;
		right = 15;
		while (left < right) {
			if (lower + map16_[left] < upper + map16_[right]) {
				lower += map16_[left];
				left++;
			} else {
				upper += map16_[right];
				right--;
			}
		}
		left *= 16;
		right = left + 15;
		while (left < right) {
			if (lower + map256_[left] < upper + map256_[right]) {
				lower += map256_[left];
				left++;
			} else {
				upper += map256_[right];
				right--;
			}
		}
		return left;
	}

};

struct median_filter_rgb_t {
	median_t r;
	median_t g;
	median_t b;
	void insert(PixelRGBA const &p)
	{
		r.insert(p.r);
		g.insert(p.g);
		b.insert(p.b);
	}
	void remove(PixelRGBA const &p)
	{
		r.remove(p.r);
		g.remove(p.g);
		b.remove(p.b);
	}
	PixelRGBA get(uint8_t a)
	{
		return PixelRGBA(r.get(), g.get(), b.get(), a);
	}
};

struct median_filter_y_t {
	median_t y;
	void insert(PixelGrayA const &p)
	{
		y.insert(p.y);
	}
	void remove(PixelGrayA const &p)
	{
		y.remove(p.y);
	}
	PixelGrayA get(uint8_t a)
	{
		return PixelGrayA(y.get(), a);
	}
};

class minimize_t {
private:
	int map256_[256];
	int map16_[16];
public:
	minimize_t()
	{
		clear();
	}
	void clear()
	{
		for (int i = 0; i < 256; i++) {
			map256_[i] = 0;
		}
		for (int i = 0; i < 16; i++) {
			map16_[i] = 0;
		}
	}
	void insert(uint8_t n)
	{
		map256_[n]++;
		map16_[n >> 4]++;
	}
	void remove(uint8_t n)
	{
		map256_[n]--;
		map16_[n >> 4]--;
	}
	uint8_t get()
	{
		int left, right;
		for (left = 0; left < 16; left++) {
			if (map16_[left] != 0) {
				left *= 16;
				right = left + 16;
				while (left < right) {
					if (map256_[left] != 0) {
						return left;
					}
					left++;
				}
				break;
			}
		}
		return 0;
	}

};

struct minimize_filter_rgb_t {
	minimize_t r;
	minimize_t g;
	minimize_t b;
	void insert(PixelRGBA const &p)
	{
		r.insert(p.r);
		g.insert(p.g);
		b.insert(p.b);
	}
	void remove(PixelRGBA const &p)
	{
		r.remove(p.r);
		g.remove(p.g);
		b.remove(p.b);
	}
	PixelRGBA get(uint8_t a)
	{
		return PixelRGBA(r.get(), g.get(), b.get(), a);
	}
};

struct minimize_filter_y_t {
	minimize_t y;
	void insert(PixelGrayA const &p)
	{
		y.insert(p.y);
	}
	void remove(PixelGrayA const &p)
	{
		y.remove(p.y);
	}
	PixelGrayA get(uint8_t a)
	{
		return PixelGrayA(y.get(), a);
	}
};


class maximize_t {
private:
	int map256_[256];
	int map16_[16];
public:
	maximize_t()
	{
		clear();
	}
	void clear()
	{
		for (int i = 0; i < 256; i++) {
			map256_[i] = 0;
		}
		for (int i = 0; i < 16; i++) {
			map16_[i] = 0;
		}
	}
	void insert(uint8_t n)
	{
		map256_[n]++;
		map16_[n >> 4]++;
	}
	void remove(uint8_t n)
	{
		map256_[n]--;
		map16_[n >> 4]--;
	}
	uint8_t get()
	{
		int left, right;
		right = 16;
		while (right > 0) {
			right--;
			if (map16_[right] != 0) {
				left = right * 16;
				right = left + 16;
				while (left < right) {
					right--;
					if (map256_[right] != 0) {
						return right;
					}
				}
				break;
			}
		}
		return 0;
	}

};

struct maximize_filter_rgb_t {
	maximize_t r;
	maximize_t g;
	maximize_t b;
	void insert(PixelRGBA const &p)
	{
		r.insert(p.r);
		g.insert(p.g);
		b.insert(p.b);
	}
	void remove(PixelRGBA const &p)
	{
		r.remove(p.r);
		g.remove(p.g);
		b.remove(p.b);
	}
	PixelRGBA get(uint8_t a)
	{
		return PixelRGBA(r.get(), g.get(), b.get(), a);
	}
};

struct maximize_filter_y_t {
	maximize_t y;
	void insert(PixelGrayA const &p)
	{
		y.insert(p.y);
	}
	void remove(PixelGrayA const &p)
	{
		y.remove(p.y);
	}
	PixelGrayA get(uint8_t a)
	{
		return PixelGrayA(y.get(), a);
	}
};



template <typename PIXEL, typename FILTER> QImage Filter(QImage image, int radius)
{
	int w = image.width();
	int h = image.height();
	if (w > 0 && h > 0) {
		std::vector<int> shape(radius * 2 + 1);
		{
			for (int y = 0; y < radius; y++) {
				double t = asin((radius - (y + 0.5)) / radius);
				double x = floor(cos(t) * radius + 0.5);
				shape[y] = x;
				shape[radius * 2 - y] = x;
			}
			shape[radius] = radius;
		}

		int sw = w + radius * 2;
		int sh = h + radius * 2;
		std::vector<PIXEL> src(sw * sh);
		PIXEL *dst = (PIXEL *)image.bits();

		for (int y = 0; y < h; y++) {
			PIXEL *d = (PIXEL *)&src[(y + radius) * sw + radius];
			PIXEL *s = (PIXEL *)image.scanLine(y);
			memcpy(d, s, sizeof(PIXEL) * w);
		}

		for (int y = 0; y < h; y++) {
			FILTER filter;
			for (int i = 0; i < radius * 2 + 1; i++) {
				for (int x = 0; x < shape[i]; x++) {
					PIXEL rgb = src[(y + i) * sw + radius + x];
					if (rgb.a > 0) {
						filter.insert(rgb);
					}
				}
			}
			for (int x = 0; x < w; x++) {
				for (int i = 0; i < radius * 2 + 1; i++) {
					PIXEL pix = src[(y + i) * sw + x + radius + shape[i]];
					if (pix.a > 0) {
						filter.insert(pix);
					}
				}

				PIXEL pix = src[(radius + y) * sw + radius + x];
				if (pix.a > 0) {
					pix = filter.get(pix.a);
				}
				dst[y * w + x] = pix;

				for (int i = 0; i < radius * 2 + 1; i++) {
					PIXEL pix = src[(y + i) * sw + x + radius - shape[i]];
					if (pix.a > 0) {
						filter.remove(pix);
					}
				}
			}
		}
	}
	return image;
}

} // namespace


QImage filter_median(QImage image, int radius)
{
	image = image.convertToFormat(QImage::Format_RGBA8888);
	return Filter<PixelRGBA, median_filter_rgb_t>(image, radius);
}

QImage filter_maximize(QImage image, int radius)
{
	image = image.convertToFormat(QImage::Format_RGBA8888);
	return Filter<PixelRGBA, maximize_filter_rgb_t>(image, radius);
}

QImage filter_minimize(QImage image, int radius)
{
	image = image.convertToFormat(QImage::Format_RGBA8888);
	return Filter<PixelRGBA, minimize_filter_rgb_t>(image, radius);
}



