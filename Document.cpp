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

void Document::renderToSinglePanel(Layer::Panel *target_panel, Layer::Panel const *input_panel, Layer const *mask_layer, QColor const &brush_color, int opacity, bool *abort)
{
	int ox = input_panel->offset_.x() - target_panel->offset_.x();
	int oy = input_panel->offset_.y() - target_panel->offset_.y();

	int dx0 = 0;
	int dy0 = 0;
	int dx1 = target_panel->image_.width();
	int dy1 = target_panel->image_.height();
	int sx0 = ox;
	int sy0 = oy;
	int sx1 = ox + input_panel->image_.width();
	int sy1 = oy + input_panel->image_.height();

	if (dx0 > sx0) { sx0 = dx0; } else { dx0 = sx0; }
	if (dx1 < sx1) { sx1 = dx1; } else { dx1 = sx1; }
	if (dy0 > sy0) { sy0 = dy0; } else { dy0 = sy0; }
	if (dy1 < sy1) { sy1 = dy1; } else { dy1 = sy1; }

	int x = sx0 - ox;
	int y = sy0 - oy;
	int w = sx1 - sx0;
	int h = sy1 - sy0;

	if (w < 1 || h < 1) return;

	QImage input_image = input_panel->image_;

	if (w > 0 && h > 0) {
		uint8_t *tmpmask = nullptr;
		QImage maskimg;
		if (mask_layer) {
			Layer::Panel panel;
			panel.offset_ = QPoint(target_panel->offset_.x() + sx0, target_panel->offset_.y() + sy0);
			panel.image_ = QImage(w, h, QImage::Format_Grayscale8);
			panel.image_.fill(Qt::black);
			renderToEachPanels_(&panel, *mask_layer, nullptr, Qt::white, 255, abort);
			maskimg = panel.image_;
		} else {
			tmpmask = (uint8_t *)alloca(w);
			memset(tmpmask, 255, w);
		}

		if (input_image.format() == QImage::Format_Grayscale8) {
			QImage const &selection = input_image;

			QColor c = brush_color.isValid() ? brush_color : Qt::white;

			uint8_t invert = 0;
			if (opacity < 0) {
				opacity = -opacity;
				invert = 255;
			}

			if (target_panel->isRGBA8888()) {
				euclase::PixelRGBA color(c.red(), c.green(), c.blue());
				for (int i = 0; i < h; i++) {
					using Pixel = euclase::PixelRGBA;
					uint8_t const *m = maskimg.isNull() ? tmpmask : maskimg.scanLine(i);
					uint8_t const *s = selection.scanLine(y + i);
					Pixel *d = reinterpret_cast<Pixel *>(target_panel->image_.scanLine(dy0 + i));
					for (int j = 0; j < w; j++) {
						color.a = opacity * (s[x + j] ^ invert) * m[j] / (255 * 255);
						d[dx0 + j] = AlphaBlend::blend_with_gamma_collection(d[dx0 + j], color);
					}
				}
			} else if (target_panel->isGrayscale8()) {
				euclase::PixelGrayA color(euclase::gray(c.red(), c.green(), c.blue()));
				uint8_t l = color.l;
				for (int i = 0; i < h; i++) {
					using Pixel = euclase::PixelGrayA;
					uint8_t const *m = maskimg.isNull() ? tmpmask : maskimg.scanLine(i);
					uint8_t const *s = reinterpret_cast<uint8_t const *>(selection.scanLine(y + i));
					uint8_t *d = reinterpret_cast<uint8_t *>(target_panel->image_.scanLine(dy0 + i));
					for (int j = 0; j < w; j++) {
						uint8_t a = opacity * (s[x + j] ^ invert) * m[j] / (255 * 255);
						d[dx0 + j] = AlphaBlend::blend(Pixel(d[dx0 + j]), Pixel(l, a)).l;
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

void Document::renderToEachPanels_(Layer::Panel *target_panel, Layer const &input_layer, Layer *mask_layer, QColor const &brush_color, int opacity, bool *abort)
{
	if (mask_layer && mask_layer->image().isNull()) {
		mask_layer = nullptr;
	}
	for (Layer::PanelPtr const &input_panel : input_layer.panels) {
		if (abort && *abort) return;
		renderToSinglePanel(target_panel, input_panel.get(), mask_layer, brush_color, opacity);
	}
}

void Document::renderToEachPanels(Layer::Panel *target_panel, Layer const &input_layer, Layer *mask_layer, QColor const &brush_color, int opacity, Synchronize *sync)
{
	if (sync) {
		sync->mutex.lock();
	}

	renderToEachPanels_(target_panel, input_layer, mask_layer, brush_color, opacity, sync ? &sync->abort : nullptr);

	if (sync) {
		sync->mutex.unlock();
	}
}

void Document::renderToLayer(Layer *target_layer, Layer const &input_layer, Layer *mask_layer, QColor const &brush_color, Synchronize *sync)
{
	target_layer->eachPanel([&](Layer::Panel *panel){
		renderToEachPanels(panel, input_layer, mask_layer, brush_color, 255, sync);
	});
}

void Document::paintToCurrentLayer(Layer const &source, QColor const &brush_color, Synchronize *sync)
{
	renderToLayer(&m->current_layer, source, selection_layer(), brush_color, sync);
}

void Document::addSelection(Layer const &source, Synchronize *sync)
{
	renderToLayer(selection_layer(), source, nullptr, Qt::white, sync);
}

void Document::subSelection(Layer const &source, Synchronize *sync)
{
	renderToLayer(selection_layer(), source, nullptr, Qt::black, sync);
}

QImage Document::renderSelection(const QRect &r, Synchronize *sync) const
{
	Layer::Panel panel;
	panel.image_ = QImage(r.width(), r.height(), QImage::Format_Grayscale8);
	panel.image_.fill(Qt::black);
	panel.offset_ = r.topLeft();
	renderToEachPanels(&panel, *selection_layer(), nullptr, QColor(), 255, sync);
	return panel.image_;
}

QImage Document::renderToLayer(const QRect &r, bool quickmask, Synchronize *sync) const
{
	Layer::Panel panel;
	panel.image_ = QImage(r.width(), r.height(), QImage::Format_RGBA8888);
	panel.offset_ = r.topLeft();
	renderToEachPanels(&panel, *current_layer(), nullptr, QColor(), 255, sync);
	if (quickmask) {
		renderToEachPanels(&panel, *selection_layer(), nullptr, QColor(255, 0, 0), -128, sync);
	}
	return panel.image_;
}
