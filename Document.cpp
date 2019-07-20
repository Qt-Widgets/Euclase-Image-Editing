#include "AlphaBlend.h"
#include "Document.h"

#include <QDebug>
#include <QElapsedTimer>
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
	return m->current_layer.width();
}

int Document::height() const
{
	return m->current_layer.height();
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

void Document::render(Layer::Panel *target_panel, Layer::Panel const *input_panel, Layer *mask_layer, QColor const &brush_color)
{
	int x = input_panel->offset_.x() - target_panel->offset_.x();
	int y = input_panel->offset_.y() - target_panel->offset_.y();

	int w = target_panel->image_.width();
	int h = target_panel->image_.height();

	int dx0 = 0;
	int dy0 = 0;
	int dx1 = w;
	int dy1 = h;
	int sx0 = x;
	int sy0 = y;
	int sx1 = x + input_panel->image_.width();
	int sy1 = y + input_panel->image_.height();

	if (dx0 > sx0) { sx0 = dx0; } else { dx0 = sx0; }
	if (dx1 < sx1) { sx1 = dx1; } else { dx1 = sx1; }
	if (dy0 > sy0) { sy0 = dy0; } else { dy0 = sy0; }
	if (dy1 < sy1) { sy1 = dy1; } else { dy1 = sy1; }

	x = sx0 - x;
	y = sy0 - y;
	w = sx1 - sx0;
	h = sy1 - sy0;

	if (w < 1 || h < 1) {
//		qDebug() << "0";
		return;
	}
//	qDebug() << "1";

	QImage input_image = input_panel->image_;

	if (w > 0 && h > 0) {
		uint8_t *tmpmask = nullptr;
		QImage maskimg;
		if (mask_layer) {
			renderMask(&maskimg, QRect(dx0, dy0, dx1 - dx0, dy1 - dy0), mask_layer->image());
		} else {
			tmpmask = (uint8_t *)alloca(w);
			memset(tmpmask, 255, w);
		}

		if (input_image.format() == QImage::Format_Grayscale8) {
			QImage const &selection = input_image;

			int opacity = 128;

			QColor const &c = brush_color;

			if (target_panel->isRGBA8888()) {
				euclase::PixelRGBA color(c.red(), c.green(), c.blue());
				for (int i = 0; i < h; i++) {
					using Pixel = euclase::PixelRGBA;
					uint8_t const *m = maskimg.isNull() ? tmpmask : maskimg.scanLine(i);
					uint8_t const *s = selection.scanLine(y + i);
					Pixel *d = reinterpret_cast<Pixel *>(target_panel->image_.scanLine(dy0 + i));
					for (int j = 0; j < w; j++) {
						color.a = opacity * s[x + j] * m[j] / (255 * 255);
						d[dx0 + j] = AlphaBlend::blend_with_gamma_collection(d[dx0 + j], color);
					}
				}
			} else if (target_panel->isGrayscale8()) {
				euclase::PixelGrayA color(euclase::gray(c.red(), c.green(), c.blue()));
				for (int i = 0; i < h; i++) {
					using Pixel = euclase::PixelGrayA;
					uint8_t const *m = maskimg.isNull() ? tmpmask : maskimg.scanLine(i);
					uint8_t const *s = reinterpret_cast<uint8_t const *>(selection.scanLine(y + i));
					uint8_t *d = reinterpret_cast<uint8_t *>(target_panel->image_.scanLine(dy0 + i));
					for (int j = 0; j < w; j++) {
						color.a = opacity * s[x + j] * m[j] / (255 * 255);
						d[dx0 + j] = AlphaBlend::blend(Pixel(d[dx0 + j]), color).l;
					}
				}
			}
			return;
		}
		if (input_image.format() == QImage::Format_RGB32) {
			input_image = input_image.convertToFormat(QImage::Format_RGBA8888);
		}
		if (input_image.format() == QImage::Format_RGBA8888) {
			if (target_panel->isRGBA8888()) {
				for (int i = 0; i < h; i++) {
					using Pixel = euclase::PixelRGBA;
					uint8_t const *m = maskimg.isNull() ? tmpmask : maskimg.scanLine(i);
					Pixel const *s = reinterpret_cast<Pixel const *>(input_image.scanLine(y + i));
					Pixel *d = reinterpret_cast<Pixel *>(target_panel->image_.scanLine(dy0 + i));
					for (int j = 0; j < w; j++) {
						euclase::PixelRGBA color = s[x + j];
						color.a = color.a * m[j] / 255;
						d[dx0 + j] = AlphaBlend::blend_with_gamma_collection(d[dx0 + j], color);
					}
				}
			} else if (target_panel->isGrayscale8()) {
			}
		}
	}
}

void Document::render(Layer *target_layer, Layer const &input_layer, Layer *mask_layer, QColor const &brush_color)
{
	target_layer->eachPanel([&](Layer::Panel *panel){
		render(input_layer, panel, mask_layer, brush_color);
	});
}

void Document::render(Layer const &input_layer, Layer::Panel *target_panel, Layer *mask_layer, QColor const &brush_color)
{
	if (mask_layer && mask_layer->image().isNull()) {
		mask_layer = nullptr;
	}

	for (Layer::PanelPtr const &input_panel : input_layer.panels) {
		render(target_panel, input_panel.get(), mask_layer, brush_color);
	}
}

void Document::paint(Layer const &sel, QColor const &brush_color)
{
	render(&m->current_layer, sel, selection_layer(), brush_color);
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
		using Pixel = euclase::PixelRGBA;
		euclase::PixelRGBA color(255, 0, 0, 128);
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

QImage Document::renderLayer(const QRect &r, const Layer &layer, QImage const &selection)
{
	QImage img;
	if (!layer.image().isNull()) {
		QRect r2 = r.translated(-layer.offset().x(), -layer.offset().y());
		img = layer.image().copy(r2);

		renderSelection(&img, r, selection);
	}
	return img;
}

QImage Document::render(const QRect &r) const
{
#if 0
	return renderLayer(r, m->current_layer, m->selection_layer.image());
#else
	Layer::Panel panel;
	panel.image_ = QImage(r.width(), r.height(), QImage::Format_RGBA8888);
	panel.offset_ = r.topLeft();
	render(*current_layer(), &panel, nullptr, QColor());
	return panel.image_;
#endif
}
