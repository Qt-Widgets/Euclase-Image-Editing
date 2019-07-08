#include "AlphaBlend.h"
#include "Document.h"

#include <QPainter>

struct Document::Private {
	Document::Layer current_layer;
	Document::Layer selection_layer;
};

Document::Document()
	: m(new Private)
{

}

Document::~Document()
{
	delete m;
}

int Document::width() const
{
	return m->current_layer.image.width();
}

int Document::height() const
{
	return m->current_layer.image.height();
}

Document::Layer *Document::current_layer()
{
	return &m->current_layer;
}

Document::Layer *Document::selection_layer()
{
	return &m->selection_layer;
}

Document::Layer *Document::current_layer() const
{
	return &m->current_layer;
}

Document::Layer *Document::selection_layer() const
{
	return &m->selection_layer;
}



void Document::blend(Layer const &selection_layer, QColor const &brush_color, Layer *target_layer, Layer *mask_layer)
{
	Q_ASSERT(selection_layer.image.format() == QImage::Format_Grayscale8);

	QImage const &selection = selection_layer.image;
	int x = selection_layer.offset.x();
	int y = selection_layer.offset.y();

	int w = target_layer->image.width();
	int h = target_layer->image.height();

	int dx0 = 0;
	int dy0 = 0;
	int dx1 = w;
	int dy1 = h;
	int sx0 = x;
	int sy0 = y;
	int sx1 = x + selection.width();
	int sy1 = y + selection.height();

	if (dx0 > sx0) { sx0 = dx0; } else { dx0 = sx0; }
	if (dx1 < sx1) { sx1 = dx1; } else { dx1 = sx1; }
	if (dy0 > sy0) { sy0 = dy0; } else { dy0 = sy0; }
	if (dy1 < sy1) { sy1 = dy1; } else { dy1 = sy1; }

	x = sx0 - x;
	y = sy0 - y;
	w = sx1 - sx0;
	h = sy1 - sy0;

	uint8_t *tmpmask = nullptr;
	QImage maskimg;
	if (mask_layer) {
		renderMask(&maskimg, QRect(dx0, dy0, dx1 - dx0, dy1 - dy0), mask_layer->image);
	} else {
		tmpmask = (uint8_t *)alloca(w);
		memset(tmpmask, 255, w);
	}

	int opacity = 128;

	QColor const &c = brush_color;

	if (target_layer->isRGBA8888()) {
		AlphaBlend::RGBA8888 color(c.red(), c.green(), c.blue());
		for (int i = 0; i < h; i++) {
			using Pixel = AlphaBlend::RGBA8888;
			uint8_t const *m = maskimg.isNull() ? tmpmask : maskimg.scanLine(i);
			uint8_t const *s = selection.scanLine(y + i);
			Pixel *d = reinterpret_cast<Pixel *>(target_layer->image.scanLine(dy0 + i));
			for (int j = 0; j < w; j++) {
				color.a = opacity * s[x + j] * m[j] / (255 * 255);
				d[dx0 + j] = AlphaBlend::blend_with_gamma_collection(d[dx0 + j], color);
			}
		}
	} else if (target_layer->isGrayscale8()) {
		AlphaBlend::GrayA88 color(euclase::gray(c.red(), c.green(), c.blue()));
		for (int i = 0; i < h; i++) {
			using Pixel = AlphaBlend::GrayA88;
			uint8_t const *m = maskimg.isNull() ? tmpmask : maskimg.scanLine(i);
			uint8_t const *s = reinterpret_cast<uint8_t const *>(selection.scanLine(y + i));
			uint8_t *d = reinterpret_cast<uint8_t *>(target_layer->image.scanLine(dy0 + i));
			for (int j = 0; j < w; j++) {
				color.a = opacity * s[x + j] * m[j] / (255 * 255);
				d[dx0 + j] = AlphaBlend::blend(Pixel(d[dx0 + j]), color).l;
			}
		}
	}
}

void Document::paint(Layer const &sel, QColor const &brush_color)
{
	blend(sel, brush_color, &m->current_layer, selection_layer());
//	blend(sel, brush_color, selection_layer(), nullptr);
}
void Document::renderSelection(QImage *dstimg, const QRect &r, QImage const &selimg)
{
	if (selimg.isNull()) return;

	QImage sel(r.width(), r.height(), QImage::Format_Grayscale8);
	sel.fill(Qt::black);
	int sx0 = r.x();
	int sy0 = r.y();
	int sx1 = r.x() + r.width();
	int sy1 = r.y() + r.height();
	int dx0 = 0;
	int dy0 = 0;
	int dx1 = sel.width();
	int dy1 = sel.height();
	if (sx0 < dx0) { dx0 = dx0 * 2 - sx0; } else if (sx0 > dx0) { sx0 = sx0 * 2 - dx0; }
	if (sy0 < dy0) { dy0 = dy0 * 2 - sy0; } else if (sy0 > dy0) { sy0 = sy0 * 2 - dy0; }
	if (sx1 < dx1) { dx1 = sx1; } else if (sx1 > dx1) { sx1 = dx1; }
	if (sy1 < dy1) { dy1 = sy1; } else if (sy1 > dy1) { sy1 = dy1; }
	{
		int w = dx1 - dx0;
		int h = dy1 - dy0;
		QPainter pr2(&sel);
		pr2.drawImage(QRect(dx0, dy0, w, h), selimg, QRect(sx0, sy0, w, h));
	}
	{
		using Pixel = AlphaBlend::RGBA8888;
		AlphaBlend::RGBA8888 color(255, 0, 0, 128);
		int opacity = color.a;

		int w = r.width();
		int h = r.height();
		for (int y = 0; y < h; y++) {
			uint8_t const *s = sel.scanLine(y);
			Pixel *d = (Pixel *)dstimg->scanLine(y);
			for (int x = 0; x < w; x++) {
				color.a = AlphaBlend::div255((255 - s[x]) * opacity);
				d[x] = AlphaBlend::blend(d[x], color);
			}
		}
	}
}

void Document::renderMask(QImage *dstimg, const QRect &r, QImage const &selimg)
{
	*dstimg = QImage(r.width(), r.height(), QImage::Format_Grayscale8);
	QPainter pr(dstimg);
	pr.drawImage(0, 0, selimg, r.x(), r.y(), r.width(), r.height());
}

QImage Document::renderLayer(const QRect &r, const Layer &layer, QImage const &selection_)
{
	QImage img;
	if (!layer.image.isNull()) {
		QRect r2 = r.translated(-layer.offset.x(), -layer.offset.y());
		img = layer.image.copy(r2);

		renderSelection(&img, r, selection_);
	}
	return img;
}

QImage Document::render(const QRect &r) const
{
	return renderLayer(r, m->current_layer, m->selection_layer.image);
}
