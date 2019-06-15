#include "resize.h"
#include <QImage>
#include <math.h>
#include <stdint.h>

namespace {

static double bicubic(double t)
{
	if (t < 0) t = -t;
	double tt = t * t;
	double ttt = t * t * t;
	const double a = -0.5;
	if (t < 1) return (a + 2) * ttt - (a + 3) * tt + 1;
	if (t < 2) return a * ttt - 5 * a * tt + 8 * a * t - 4 * a;
	return 0;
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
	uint8_t v, a;
	PixelGrayA()
		: v(0)
		, a(0)
	{
	}
	PixelGrayA(uint8_t v, uint8_t a = 255)
		: v(v)
		, a(a)
	{
	}
};

class FPixelRGB {
public:
	double r;
	double g;
	double b;
	FPixelRGB()
		: r(0)
		, g(0)
		, b(0)
	{
	}
	FPixelRGB(double r, double g, double b)
		: r(r)
		, g(g)
		, b(b)
	{
	}
	FPixelRGB(PixelRGBA const &src)
		: r(src.r)
		, g(src.g)
		, b(src.b)
	{
	}
	FPixelRGB operator + (FPixelRGB const &right) const
	{
		return FPixelRGB(r + right.r, g + right.g, b + right.b);
	}
	FPixelRGB operator * (double t) const
	{
		return FPixelRGB(r * t, g * t, b * t);
	}
	void operator += (FPixelRGB const &o)
	{
		r += o.r;
		g += o.g;
		b += o.b;
	}
	void add(FPixelRGB const &p, double v)
	{
		r += p.r * v;
		g += p.g * v;
		b += p.b * v;
	}
	void sub(FPixelRGB const &p, double v)
	{
		r -= p.r * v;
		g -= p.g * v;
		b -= p.b * v;
	}

	void operator *= (double t)
	{
		r *= t;
		g *= t;
		b *= t;
	}
	int r8() const
	{
		if (r <= 0) return 0;
		if (r >= 255) return 255;
		return (int)r;
	}
	int g8() const
	{
		if (g < 0) return 0;
		if (g > 255) return 255;
		return (int)g;
	}
	int b8() const
	{
		if (b < 0) return 0;
		if (b > 255) return 255;
		return (int)b;
	}
	PixelRGBA color(double amount) const
	{
		if (amount == 1) {
			return PixelRGBA(r8(), g8(), b8());
		} else if (amount == 0) {
			return PixelRGBA(0, 0, 0);
		}
		double m = 1 / amount;
		FPixelRGB p = *this * m;
		return PixelRGBA(p.r8(), p.g8(), p.b8());
	}
	PixelRGBA toPixelRGBA() const
	{
		return PixelRGBA(r8(), g8(), b8());
	}
};

class FPixelGray {
public:
	double v;
	FPixelGray()
		: v(0)
	{
	}
	FPixelGray(double v)
		: v(v)
	{
	}
	FPixelGray(PixelGrayA const &src)
		: v(src.v)
	{
	}
	FPixelGray operator + (FPixelGray const &right) const
	{
		return FPixelGray(v + right.v);
	}
	FPixelGray operator * (double t) const
	{
		return FPixelGray(v * t);
	}
	void operator += (FPixelGray const &o)
	{
		v += o.v;
	}
	void add(FPixelGray const &p, double v)
	{
		v += p.v * v;
	}
	void sub(FPixelGray const &p, double v)
	{
		v -= p.v * v;
	}

	void operator *= (double t)
	{
		v *= t;
	}
	int v8() const
	{
		if (v <= 0) return 0;
		if (v >= 255) return 255;
		return (int)v;
	}
	PixelGrayA color(double amount) const
	{
		if (amount == 1) {
			return PixelGrayA(v8());
		} else if (amount == 0) {
			return PixelGrayA(0);
		}
		double m = 1 / amount;
		FPixelGray p = *this * m;
		return PixelGrayA(p.v8());
	}
	PixelGrayA toPixelGrayA() const
	{
		return PixelGrayA(v8());
	}
};

class FPixelRGBA {
public:
	double r;
	double g;
	double b;
	double a;
	FPixelRGBA()
		: r(0)
		, g(0)
		, b(0)
		, a(0)
	{
	}
	FPixelRGBA(double r, double g, double b,  double a = 255)
		: r(r)
		, g(g)
		, b(b)
		, a(a)
	{
	}
	FPixelRGBA(PixelRGBA const &src)
		: r(src.r)
		, g(src.g)
		, b(src.b)
		, a(src.a)
	{
	}
	FPixelRGBA operator + (FPixelRGBA const &right) const
	{
		return FPixelRGBA(r + right.r, g + right.g, b + right.b);
	}
	FPixelRGBA operator * (double t) const
	{
		return FPixelRGBA(r * t, g * t, b * t);
	}
	void operator += (FPixelRGBA const &o)
	{
		r += o.r;
		g += o.g;
		b += o.b;
	}
	void operator *= (double t)
	{
		r *= t;
		g *= t;
		b *= t;
	}
	void add(FPixelRGBA const &p, double v)
	{
		v *= p.a;
		a += v;
		v /= 255;
		r += p.r * v;
		g += p.g * v;
		b += p.b * v;
	}
	void sub(FPixelRGBA const &p, double v)
	{
		v *= p.a;
		a -= v;
		v /= 255;
		r -= p.r * v;
		g -= p.g * v;
		b -= p.b * v;
	}
	int r8() const
	{
		if (r <= 0) return 0;
		if (r >= 255) return 255;
		return (int)r;
	}
	int g8() const
	{
		if (g < 0) return 0;
		if (g > 255) return 255;
		return (int)g;
	}
	int b8() const
	{
		if (b < 0) return 0;
		if (b > 255) return 255;
		return (int)b;
	}
	int a8() const
	{
		if (a < 0) return 0;
		if (a > 255) return 255;
		return (int)a;
	}
	PixelRGBA color(double amount) const
	{
		if (amount == 0) {
			return PixelRGBA(0, 0, 0, 0);
		}
		FPixelRGBA pixel(*this);
		pixel *= (255.0f / pixel.a);
		pixel.a = pixel.a / amount;
		if (pixel.a < 0) pixel.a = 0;
		else if (pixel.a > 255) pixel.a = 255;
		return PixelRGBA(pixel.r8(), pixel.g8(), pixel.b8(), pixel.a8());
	}
	PixelRGBA toPixelRGBAa(double amount) const
	{
		return color(amount);
	}
};

class FPixelGrayA {
public:
	double v;
	double a;
	FPixelGrayA()
		: v(0)
		, a(0)
	{
	}
	FPixelGrayA(double v,  double a = 255)
		: v(v)
		, a(a)
	{
	}
	FPixelGrayA(PixelGrayA const &src)
		: v(src.v)
		, a(src.a)
	{
	}
	FPixelGrayA operator + (FPixelGrayA const &right) const
	{
		return FPixelGrayA(v + right.v);
	}
	FPixelGrayA operator * (double t) const
	{
		return FPixelGrayA(v * t);
	}
	void operator += (FPixelGrayA const &o)
	{
		v += o.v;
	}
	void operator *= (double t)
	{
		v *= t;
	}
	void add(FPixelGrayA const &p, double v)
	{
		v *= p.a;
		a += v;
		v /= 255;
		v += p.v * v;
	}
	void sub(FPixelGrayA const &p, double v)
	{
		v *= p.a;
		a -= v;
		v /= 255;
		v -= p.v * v;
	}
	int v8() const
	{
		if (v <= 0) return 0;
		if (v >= 255) return 255;
		return (int)v;
	}
	int a8() const
	{
		if (a < 0) return 0;
		if (a > 255) return 255;
		return (int)a;
	}
	PixelGrayA color(double amount) const
	{
		if (amount == 0) {
			return PixelGrayA(0, 0);
		}
		FPixelGrayA pixel(*this);
		pixel *= (255.0f / pixel.a);
		pixel.a = pixel.a / amount;
		if (pixel.a < 0) pixel.a = 0;
		else if (pixel.a > 255) pixel.a = 255;
		return PixelGrayA(pixel.v8(), pixel.a8());
	}
	PixelGrayA toPixelGrayAa(double amount) const
	{
		return color(amount);
	}
};

QImage resizeNearestNeighbor(QImage const &image, int dst_w, int dst_h)
{
	const int src_w = image.width();
	const int src_h = image.height();
	QImage newimg(dst_w, dst_h, QImage::Format_RGBA8888);
	for (int y = 0; y < dst_h; y++) {
		double fy = (double)y * src_h / dst_h;
		PixelRGBA *dst = (PixelRGBA *)newimg.scanLine(y);
		PixelRGBA const *src = (PixelRGBA const *)image.scanLine((int)fy);
		double mul = (double)src_w / dst_w;
		for (int x = 0; x < dst_w; x++) {
			double fx = (double)x * mul;
			dst[x] = src[(int)fx];
		}
	}
	return std::move(newimg);
}

template <typename PIXEL>
QImage resizeAveragingT(QImage const &image, int dst_w, int dst_h)
{
	const int src_w = image.width();
	const int src_h = image.height();
	QImage newimg(dst_w, dst_h, QImage::Format_RGBA8888);
	for (int y = 0; y < dst_h; y++) {
		double lo_y = (double)y * src_h / dst_h;
		double hi_y = (double)(y + 1) * src_h / dst_h;
		PixelRGBA *dst = (PixelRGBA *)newimg.scanLine(y);
		double mul = (double)src_w / dst_w;
		for (int x = 0; x < dst_w; x++) {
			double lo_x = (double)x * mul;
			double hi_x = (double)(x + 1) * mul;
			int lo_iy = (int)lo_y;
			int hi_iy = (int)hi_y;
			int lo_ix = (int)lo_x;
			int hi_ix = (int)hi_x;
			PIXEL pixel;
			double volume = 0;
			for (int sy = lo_iy; sy <= hi_iy; sy++) {
				double vy = 1;
				if (sy < src_h) {
					if (lo_iy == hi_iy) {
						vy = hi_y - lo_y;
					} else if (sy == lo_iy) {
						vy = 1 - (lo_y - sy);
					} else if (sy == hi_iy) {
						vy = hi_y - sy;
					}
				}
				PixelRGBA const *src = (PixelRGBA const *)image.scanLine(sy < src_h ? sy : (src_h - 1));
				for (int sx = lo_ix; sx <= hi_ix; sx++) {
					PIXEL p = src[sx < src_w ? sx : (src_w - 1)];
					double vx = 1;
					if (sx < src_w) {
						if (lo_ix == hi_ix) {
							vx = hi_x - lo_x;
						} else if (sx == lo_ix) {
							vx = 1 - (lo_x - sx);
						} else if (sx == hi_ix) {
							vx = hi_x - sx;
						}
					}
					double v = vy * vx;
					pixel.add(p, v);
					volume += v;
				}
			}
			dst[x] = pixel.color(volume);
		}
	}
	return std::move(newimg);
}

template <typename PIXEL>
QImage resizeAveragingHT(QImage const &image, int dst_w)
{
	const int src_w = image.width();
	const int src_h = image.height();
	QImage newimg(dst_w, src_h, QImage::Format_RGBA8888);
	for (int y = 0; y < src_h; y++) {
		PixelRGBA *dst = (PixelRGBA *)newimg.scanLine(y);
		for (int x = 0; x < dst_w; x++) {
			double lo_x = (double)x * src_w / dst_w;
			double hi_x = (double)(x + 1) * src_w / dst_w;
			int lo_ix = (int)lo_x;
			int hi_ix = (int)hi_x;
			PIXEL pixel;
			double volume = 0;
			PixelRGBA const *src = (PixelRGBA const *)image.scanLine(y < src_h ? y : (src_h - 1));
			for (int sx = lo_ix; sx <= hi_ix; sx++) {
				PIXEL p = src[sx < src_w ? sx : (src_w - 1)];
				double v = 1;
				if (sx < src_w) {
					if (lo_ix == hi_ix) {
						v = hi_x - lo_x;
					} else if (sx == lo_ix) {
						v = 1 - (lo_x - sx);
					} else if (sx == hi_ix) {
						v = hi_x - sx;
					}
				}
				pixel.add(p, v);
				volume += v;
			}
			dst[x] = pixel.color(volume);
		}
	}
	return std::move(newimg);
}

template <typename PIXEL>
QImage resizeAveragingVT(QImage const &image, int dst_h)
{
	const int src_w = image.width();
	const int src_h = image.height();
	QImage newimg(src_w, dst_h, QImage::Format_RGBA8888);
	for (int y = 0; y < dst_h; y++) {
		double lo_y = (double)y * src_h / dst_h;
		double hi_y = (double)(y + 1) * src_h / dst_h;
		PixelRGBA *dst = (PixelRGBA *)newimg.scanLine(y);
		for (int x = 0; x < src_w; x++) {
			int lo_iy = (int)lo_y;
			int hi_iy = (int)hi_y;
			PIXEL pixel;
			double volume = 0;
			for (int sy = lo_iy; sy <= hi_iy; sy++) {
				double v = 1;
				if (sy < src_h) {
					if (lo_iy == hi_iy) {
						v = hi_y - lo_y;
					} else if (sy == lo_iy) {
						v = 1 - (lo_y - sy);
					} else if (sy == hi_iy) {
						v = hi_y - sy;
					}
				}
				PixelRGBA const *src = (PixelRGBA const *)image.scanLine(sy < src_h ? sy : (src_h - 1));
				PIXEL p = src[x];
				pixel.add(p, v);
				volume += v;
			}
			dst[x] = pixel.color(volume);
		}
	}
	return std::move(newimg);
}

struct bilinear_t {
	int i0, i1;
	double v0, v1;
};

template <typename PIXEL>
QImage resizeBilinearT(QImage const &image, int dst_w, int dst_h)
{
	const int src_w = image.width();
	const int src_h = image.height();
	QImage newimg(dst_w, dst_h, QImage::Format_RGBA8888);

	std::vector<bilinear_t> lut(dst_w);
	bilinear_t *lut_p = &lut[0];
	for (int x = 0; x < dst_w; x++) {
		double tx = (double)x * src_w / dst_w - 0.5;
		int x0, x1;
		if (tx < 0) {
			x0 = x1 = 0;
			tx = 0;
		} else {
			x0 = x1 = (int)tx;
			if (x0 + 1 < src_w) {
				x1 = x0 + 1;
				tx -= x0;
			} else {
				x0 = x1 = src_w - 1;
				tx = 0;
			}
		}
		lut_p[x].i0 = x0;
		lut_p[x].i1 = x1;
		lut_p[x].v1 = tx;
		lut_p[x].v0 = 1 - tx;
	}

	for (int y = 0; y < dst_h; y++) {
		double yt = (double)y * src_h / dst_h - 0.5;
		int y0, y1;
		if (yt < 0) {
			y0 = y1 = 0;
			yt = 0;
		} else {
			y0 = y1 = (int)yt;
			if (y0 + 1 < src_h) {
				y1 = y0 + 1;
				yt -= y0;
			} else {
				y0 = y1 = src_h - 1;
				yt = 0;
			}
		}
		double ys = 1 - yt;
		PixelRGBA *dst = (PixelRGBA *)newimg.scanLine(y);
		PixelRGBA const *src1 = (PixelRGBA const *)image.scanLine(y0);
		PixelRGBA const *src2 = (PixelRGBA const *)image.scanLine(y1);
		for (int x = 0; x < dst_w; x++) {
			double a11 = lut_p[x].v0 * ys;
			double a12 = lut_p[x].v1 * ys;
			double a21 = lut_p[x].v0 * yt;
			double a22 = lut_p[x].v1 * yt;
			PIXEL pixel;
			pixel.add(PIXEL(src1[lut_p[x].i0]), a11);
			pixel.add(PIXEL(src1[lut_p[x].i1]), a12);
			pixel.add(PIXEL(src2[lut_p[x].i0]), a21);
			pixel.add(PIXEL(src2[lut_p[x].i1]), a22);
			dst[x] = pixel.color(a11 + a12 + a21 + a22);
		}
	}
	return std::move(newimg);
}

template <typename PIXEL>
QImage resizeBilinearHT(QImage const &image, int dst_w)
{
	const int src_w = image.width();
	const int src_h = image.height();
	QImage newimg(dst_w, src_h, QImage::Format_RGBA8888);
	for (int y = 0; y < src_h; y++) {
		PixelRGBA *dst = (PixelRGBA *)newimg.scanLine(y);
		PixelRGBA const *src = (PixelRGBA const *)image.scanLine(y);
		double mul = (double)src_w / dst_w;
		for (int x = 0; x < dst_w; x++) {
			double xt = (double)x * mul - 0.5;
			int x0, x1;
			if (xt < 0) {
				x0 = x1 = 0;
				xt = 0;
			} else {
				x0 = x1 = (int)xt;
				if (x0 + 1 < src_w) {
					x1 = x0 + 1;
					xt -= x0;
				} else {
					x0 = x1 = src_w - 1;
					xt = 0;
				}
			}
			double xs = 1 - xt;
			PIXEL p1(src[x0]);
			PIXEL p2(src[x1]);
			PIXEL p;
			p.add(p1, xs);
			p.add(p2, xt);
			dst[x] = p.color(1);
		}
	}
	return std::move(newimg);
}

template <typename PIXEL>
QImage resizeBilinearVT(QImage const &image, int dst_h)
{
	const int src_w = image.width();
	const int src_h = image.height();
	QImage newimg(src_w, dst_h, QImage::Format_RGBA8888);
	for (int y = 0; y < dst_h; y++) {
		double yt = (double)y * src_h / dst_h - 0.5;
		int y0, y1;
		if (yt < 0) {
			y0 = y1 = 0;
			yt = 0;
		} else {
			y0 = y1 = (int)yt;
			if (y0 + 1 < src_h) {
				y1 = y0 + 1;
				yt -= y0;
			} else {
				y0 = y1 = src_h - 1;
				yt = 0;
			}
		}
		double ys = 1 - yt;
		PixelRGBA *dst = (PixelRGBA *)newimg.scanLine(y);
		PixelRGBA const *src1 = (PixelRGBA const *)image.scanLine(y0);
		PixelRGBA const *src2 = (PixelRGBA const *)image.scanLine(y1);
		for (int x = 0; x < src_w; x++) {
			PIXEL p1(src1[x]);
			PIXEL p2(src2[x]);
			PIXEL p;
			p.add(p1, ys);
			p.add(p2, yt);
			dst[x] = p.color(1);
		}
	}
	return std::move(newimg);
}

typedef double (*bicubic_lut_t)[4];

static bicubic_lut_t makeBicubicLookupTable(int src, int dst, std::vector<double> *out)
{
	out->resize(dst * 4);
	double (*lut)[4] = (double (*)[4])&(*out)[0];
	for (int x = 0; x < dst; x++) {
		double sx = (double)x * src / dst - 0.5;
		int ix = (int)floor(sx);
		double tx = sx - ix;
		for (int x2 = -1; x2 <= 2; x2++) {
			int x3 = ix + x2;
			if (x3 >= 0 && x3 < src) {
				lut[x][x2 + 1] = bicubic(x2 - tx);
			}
		}
	}
	return lut;
}

template <typename PIXEL>
QImage resizeBicubicT(QImage const &image, int dst_w, int dst_h)
{
	const int src_w = image.width();
	const int src_h = image.height();
	QImage newimg(dst_w, dst_h, QImage::Format_RGBA8888);

	std::vector<double> bicubic_lut_x;
	std::vector<double> bicubic_lut_y;
	bicubic_lut_t bicubic_lut_x_p = makeBicubicLookupTable(src_w, dst_w, &bicubic_lut_x);
	bicubic_lut_t bicubic_lut_y_p = makeBicubicLookupTable(src_h, dst_h, &bicubic_lut_y);

	for (int y = 0; y < dst_h; y++) {
		PixelRGBA *dst = (PixelRGBA *)newimg.scanLine(y);
		double sy = (double)y * src_h / dst_h - 0.5;
		int iy = (int)floor(sy);
		for (int x = 0; x < dst_w; x++) {
			double sx = (double)x * src_w / dst_w - 0.5;
			int ix = (int)floor(sx);
			PIXEL pixel;
			double amount = 0;
			for (int y2 = -1; y2 <= 2; y2++) {
				int y3 = iy + y2;
				if (y3 >= 0 && y3 < src_h) {
					double vy = bicubic_lut_y_p[y][y2 + 1];
					PixelRGBA const *src = (PixelRGBA const *)image.scanLine(y3);
					for (int x2 = -1; x2 <= 2; x2++) {
						int x3 = ix + x2;
						if (x3 >= 0 && x3 < src_w) {
							double vx = bicubic_lut_x_p[x][x2 + 1];
							PIXEL p = src[x3];
							double v = vx * vy;
							pixel.add(p, v);
							amount += v;
						}
					}
				}
			}
			dst[x] = pixel.color(amount);
		}
	}
	return std::move(newimg);
}

template <typename PIXEL>
QImage resizeBicubicHT(QImage const &image, int dst_w)
{
	const int src_w = image.width();
	const int src_h = image.height();
	QImage newimg(dst_w, src_h, QImage::Format_RGBA8888);

	std::vector<double> bicubic_lut_x;
	bicubic_lut_t bicubic_lut_x_p = makeBicubicLookupTable(src_w, dst_w, &bicubic_lut_x);

	for (int y = 0; y < src_h; y++) {
		PixelRGBA *dst = (PixelRGBA *)newimg.scanLine(y);
		PixelRGBA const *src = (PixelRGBA const *)image.scanLine(y);
		for (int x = 0; x < dst_w; x++) {
			PIXEL pixel;
			double volume = 0;
			double sx = (double)x * src_w / dst_w - 0.5;
			int ix = (int)floor(sx);
			for (int x2 = -1; x2 <= 2; x2++) {
				int x3 = ix + x2;
				if (x3 >= 0 && x3 < src_w) {
					double v = bicubic_lut_x_p[x][x2 + 1];
					PIXEL p = src[x3];
					pixel.add(p, v);
					volume += v;
				}
			}
			dst[x] = pixel.color(volume);
		}
	}
	return std::move(newimg);
}

template <typename PIXEL>
QImage resizeBicubicVT(QImage const &image, int dst_h)
{
	const int src_w = image.width();
	const int src_h = image.height();
	QImage newimg(src_w, dst_h, QImage::Format_RGBA8888);

	std::vector<double> bicubic_lut_y;
	bicubic_lut_t bicubic_lut_y_p = makeBicubicLookupTable(src_h, dst_h, &bicubic_lut_y);

	for (int x = 0; x < src_w; x++) {
		for (int y = 0; y < dst_h; y++) {
			PixelRGBA *dst = (PixelRGBA *)newimg.scanLine(y) + x;
			PIXEL pixel;
			double volume = 0;
			double sy = (double)y * src_h / dst_h - 0.5;
			int iy = (int)floor(sy);
			for (int y2 = -1; y2 <= 2; y2++) {
				int y3 = iy + y2;
				if (y3 >= 0 && y3 < src_h) {
					double v = bicubic_lut_y_p[y][y2 + 1];
					PIXEL p = ((PixelRGBA const *)image.scanLine(y3))[x];
					pixel.add(p, v);
					volume += v;
				}
			}
			*dst = pixel.color(volume);
		}
	}
	return std::move(newimg);
}

//

template <typename PIXEL, typename FPIXEL> QImage BlurFilter(QImage image, int radius)
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

		std::vector<PIXEL> dst_(w * h);

		for (int y = 0; y < h; y++) {
			FPIXEL pixel;
			for (int i = 0; i < radius * 2 + 1; i++) {
				int y2 = y + i - radius;
				if (y2 >= 0 && y2 < h) {
					PIXEL const *s = (PIXEL const *)image.scanLine(y2);
					for (int x = 0; x < shape[i]; x++) {
						if (x < w) {
							PIXEL pix = s[x];
							pixel.add(pix, 1);
						}
					}
				}
			}
			for (int x = 0; x < w; x++) {
				for (int i = 0; i < radius * 2 + 1; i++) {
					int y2 = y + i - radius;
					if (y2 >= 0 && y2 < h) {
						PIXEL const *s = (PIXEL const *)image.scanLine(y2);
						int x2 = x + shape[i];
						if (x2 < w) {
							PIXEL pix = s[x2];
							pixel.add(pix, 1);
						}
					}
				}

				{
					PIXEL const *s = (PIXEL const *)image.scanLine(y);
					PIXEL pix = s[x];
					pix = pixel.color(1);
					dst_[y * w + x] = pix;
				}

				for (int i = 0; i < radius * 2 + 1; i++) {
					int y2 = y + i - radius;
					if (y2 >= 0 && y2 < h) {
						PIXEL const *s = (PIXEL const *)image.scanLine(y2);
						int x2 = x - shape[i];
						if (x2 >= 0) {
							PIXEL pix = s[x2];
							pixel.sub(pix, 1);
						}
					}
				}
			}
		}

		for (int y = 0; y < h; y++) {
			PIXEL *s = &dst_[y * w];
			PIXEL *d = (PIXEL *)image.scanLine(y);
			memcpy(d, s, sizeof(PIXEL) * w);
		}
	}
	return image;
}

}

QImage resizeImage(QImage image, int dst_w, int dst_h, EnlargeMethod method, bool alphachannel)
{
	if (dst_w > 0 && dst_h > 0) {
		int w, h;
		w = image.width();
		h = image.height();
		if (w != dst_w || h != dst_h) {
			if (dst_w < w || dst_h < h) {
				if (dst_w < w && dst_h < h) {
					if (alphachannel) {
						image = resizeAveragingT<FPixelRGBA>(image, dst_w, dst_h);
					} else {
						image = resizeAveragingT<FPixelRGB>(image, dst_w, dst_h);
					}
				} else if (dst_w < w) {
					if (alphachannel) {
						image = resizeAveragingHT<FPixelRGBA>(image, dst_w);
					} else {
						image = resizeAveragingHT<FPixelRGB>(image, dst_w);
					}
				} else if (dst_h < h) {
					if (alphachannel) {
						image = resizeAveragingVT<FPixelRGBA>(image, dst_h);
					} else {
						image = resizeAveragingVT<FPixelRGB>(image, dst_h);
					}
				}
			}
			w = image.width();
			h = image.height();
			if (dst_w > w || dst_h > h) {
				if (method == EnlargeMethod::Bilinear) {
					if (dst_w > w && dst_h > h) {
						if (alphachannel) {
							image = resizeBilinearT<FPixelRGBA>(image, dst_w, dst_h);
						} else {
							image = resizeBilinearT<FPixelRGB>(image, dst_w, dst_h);
						}
					} else if (dst_w > w) {
						if (alphachannel) {
							image = resizeBilinearHT<FPixelRGBA>(image, dst_w);
						} else {
							image = resizeBilinearHT<FPixelRGB>(image, dst_w);
						}
					} else if (dst_h > h) {
						if (alphachannel) {
							image = resizeBilinearVT<FPixelRGBA>(image, dst_h);
						} else {
							image = resizeBilinearVT<FPixelRGB>(image, dst_h);
						}
					}
				} else if (method == EnlargeMethod::Bicubic) {
					if (dst_w > w && dst_h > h) {
						if (alphachannel) {
							image = resizeBicubicT<FPixelRGBA>(image, dst_w, dst_h);
						} else {
							image = resizeBicubicT<FPixelRGB>(image, dst_w, dst_h);
						}
					} else if (dst_w > w) {
						if (alphachannel) {
							image = resizeBicubicHT<FPixelRGBA>(image, dst_w);
						} else {
							image = resizeBicubicHT<FPixelRGB>(image, dst_w);
						}
					} else if (dst_h > h) {
						if (alphachannel) {
							image = resizeBicubicVT<FPixelRGBA>(image, dst_h);
						} else {
							image = resizeBicubicVT<FPixelRGB>(image, dst_h);
						}
					}
				} else {
					image = resizeNearestNeighbor(image, dst_w, dst_h);
				}
			}
		}
		return std::move(image);
	}
	return QImage();
}

QImage filter_blur(QImage image, int radius)
{
	if (image.format() == QImage::Format_Grayscale8) {
		return BlurFilter<PixelGrayA, FPixelGrayA>(image, radius);
	}
	image = image.convertToFormat(QImage::Format_RGBA8888);
	return BlurFilter<PixelRGBA, FPixelRGBA>(image, radius);
}
