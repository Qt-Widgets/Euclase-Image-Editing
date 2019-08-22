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

void Document::renderToSinglePanel(Layer::Image *target_panel, QPoint const &target_offset, Layer::Image const *input_panel, QPoint const &input_offset, Layer const *mask_layer, QColor const &brush_color, int opacity, bool *abort)
{
#if 0
	QPoint org = input_panel->offset() + input_offset - (target_panel->offset() + target_offset);

	int dx0 = 0;
	int dy0 = 0;
	int dx1 = target_panel->image_.width();
	int dy1 = target_panel->image_.height();
	int sx0 = org.x();
	int sy0 = org.y();
	int sx1 = org.x() + input_panel->image_.width();
	int sy1 = org.y() + input_panel->image_.height();

	if (dx0 > sx0) { sx0 = dx0; } else { dx0 = sx0; }
	if (dx1 < sx1) { sx1 = dx1; } else { dx1 = sx1; }
	if (dy0 > sy0) { sy0 = dy0; } else { dy0 = sy0; }
	if (dy1 < sy1) { sy1 = dy1; } else { dy1 = sy1; }

	int x = sx0 - org.x();
	int y = sy0 - org.y();
	int w = sx1 - sx0;
	int h = sy1 - sy0;

	if (w < 1 || h < 1) return;
#else
	const QPoint dst_org = target_offset + target_panel->offset();
	const QPoint src_org = input_offset + input_panel->offset();
	int dx0 = dst_org.x();
	int dy0 = dst_org.y();
	int dx1 = dx0 + target_panel->width();
	int dy1 = dy0 + target_panel->height();
	int sx0 = src_org.x();
	int sy0 = src_org.y();
	int sx1 = sx0 + input_panel->width();
	int sy1 = sy0 + input_panel->height();
	if (sx1 <= dx0) return;
	if (sy1 <= dy0) return;
	if (dx1 <= sx0) return;
	if (dy1 <= sy0) return;
	const int x0 = std::max(dx0, sx0);
	const int y0 = std::max(dy0, sy0);
	const int x1 = std::min(dx1, sx1);
	const int y1 = std::min(dy1, sy1);
	const int w = x1 - x0;
	const int h = y1 - y0;
#endif

	if (w < 1 || h < 1) return;

	QImage input_image = input_panel->image_;

	const int dx = x0 - dst_org.x();
	const int dy = y0 - dst_org.y();
	const int sx = x0 - src_org.x();
	const int sy = y0 - src_org.y();
	uint8_t *tmpmask = nullptr;
	QImage maskimg;
	if (mask_layer && !mask_layer->panels.empty()) {
		Layer::Image panel;
		panel.offset_ = QPoint(x0, y0);
		panel.image_ = QImage(w, h, QImage::Format_Grayscale8);
		panel.image_.fill(Qt::black);
		renderToEachPanels_(&panel, target_offset, *mask_layer, nullptr, Qt::white, 255, abort);
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
				uint8_t const *msk = maskimg.isNull() ? tmpmask : maskimg.scanLine(i);
				uint8_t const *src = selection.scanLine(sy + i);
				Pixel *dst = reinterpret_cast<Pixel *>(target_panel->image_.scanLine(dy + i));
				for (int j = 0; j < w; j++) {
					color.a = opacity * (src[sx + j] ^ invert) * msk[j] / (255 * 255);
					dst[dx + j] = AlphaBlend::blend_with_gamma_collection(dst[dx + j], color);
				}
			}
		} else if (target_panel->isGrayscale8()) {
			euclase::PixelGrayA color(euclase::gray(c.red(), c.green(), c.blue()));
			uint8_t l = color.l;
			for (int i = 0; i < h; i++) {
				using Pixel = euclase::PixelGrayA;
				uint8_t const *msk = maskimg.isNull() ? tmpmask : maskimg.scanLine(i);
				uint8_t const *src = reinterpret_cast<uint8_t const *>(selection.scanLine(sy + i));
				uint8_t *dst = reinterpret_cast<uint8_t *>(target_panel->image_.scanLine(dy + i));
				for (int j = 0; j < w; j++) {
					uint8_t a = opacity * (src[sx + j] ^ invert) * msk[j] / (255 * 255);
					dst[dx + j] = AlphaBlend::blend(Pixel(dst[dx + j]), Pixel(l, a)).l;
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
				uint8_t const *msk = maskimg.isNull() ? tmpmask : maskimg.scanLine(i);
				Pixel const *src = reinterpret_cast<Pixel const *>(input_image.scanLine(sy + i));
				Pixel *dst = reinterpret_cast<Pixel *>(target_panel->image_.scanLine(dy + i));
				for (int j = 0; j < w; j++) {
					euclase::PixelRGBA color = src[sx + j];
					color.a = color.a * msk[j] / 255;
					dst[dx + j] = AlphaBlend::blend_with_gamma_collection(dst[dx + j], color);
				}
			}
		} else if (target_panel->isGrayscale8()) {
		}
	}
}

void Document::renderToEachPanels_(Layer::Image *target_panel, QPoint const &target_offset, Layer const &input_layer, Layer *mask_layer, QColor const &brush_color, int opacity, bool *abort)
{
	if (mask_layer && mask_layer->panels.empty()) {
		mask_layer = nullptr;
	}
	for (Layer::PanelPtr const &input_panel : input_layer.panels) {
		if (abort && *abort) return;
		renderToSinglePanel(target_panel, target_offset, input_panel.image(), input_layer.offset(), mask_layer, brush_color, opacity);
	}
}

void Document::renderToEachPanels(Layer::Image *target_panel, QPoint const &target_offset, Layer const &input_layer, Layer *mask_layer, QColor const &brush_color, int opacity, QMutex *sync, bool *abort)
{
	if (sync) {
		QMutexLocker lock(sync);
		renderToEachPanels(target_panel, target_offset, input_layer, mask_layer, brush_color, opacity, nullptr, abort);
		return;
	}

	renderToEachPanels_(target_panel, target_offset, input_layer, mask_layer, brush_color, opacity, abort);
}

void Document::renderToLayer(Layer *target_layer, Layer const &input_layer, Layer *mask_layer, QColor const &brush_color, QMutex *sync, bool *abort)
{
	for (Layer::PanelPtr const &input_panel : input_layer.panels) {
		if (input_panel.isImage()) {
			int count = 0;
			for (Layer::PanelPtr &panel : target_layer->panels) {
				if (abort && *abort) return;
				if (sync) sync->lock();
				if (panel.isImage()) {
					renderToSinglePanel(panel.image(), target_layer->offset(), input_panel.image(), input_layer.offset(), mask_layer, brush_color, 255, abort);
				}
				if (sync) sync->unlock();
				count++;
			}
			if (count == 0) {
				Layer::PanelPtr panel = Layer::PanelPtr::makePanel();
				panel->image_ = input_panel->image_.copy();
				panel->offset_ = input_panel->offset();
				if (sync) sync->lock();
				target_layer->panels.push_back(panel);
				if (sync) sync->unlock();
			}
		}
	}
}

void Document::clearSelection(QMutex *sync)
{
	selection_layer()->clear(sync);
}

void Document::paintToCurrentLayer(Layer const &source, QColor const &brush_color, QMutex *sync, bool *abort)
{
	renderToLayer(&m->current_layer, source, selection_layer(), brush_color, sync, abort);
}

void Document::addSelection(Layer const &source, QMutex *sync, bool *abort)
{
	renderToLayer(selection_layer(), source, nullptr, Qt::white, sync, abort);
}

void Document::subSelection(Layer const &source, QMutex *sync, bool *abort)
{
	renderToLayer(selection_layer(), source, nullptr, Qt::black, sync, abort);
}

QImage Document::renderSelection(const QRect &r, QMutex *sync, bool *abort) const
{
	Layer::Image panel;
	panel.image_ = QImage(r.width(), r.height(), QImage::Format_Grayscale8);
	panel.image_.fill(Qt::black);
	panel.offset_ = r.topLeft();
	renderToEachPanels(&panel, QPoint(), *selection_layer(), nullptr, QColor(), 255, sync, abort);
	return panel.image_;
}

QImage Document::renderToLayer(const QRect &r, bool quickmask, QMutex *sync, bool *abort) const
{
	Layer::Image panel;
	panel.image_ = QImage(r.width(), r.height(), QImage::Format_RGBA8888);
	panel.offset_ = r.topLeft();
	renderToEachPanels(&panel, QPoint(), *current_layer(), nullptr, QColor(), 255, sync, abort);
	if (quickmask) {
		renderToEachPanels(&panel, QPoint(), *selection_layer(), nullptr, QColor(255, 0, 0), -128, sync, abort);
	}
	return panel.image_;
}

QRect Document::Layer::rect() const
{
	QRect rect;
	const_cast<Layer *>(this)->eachPanel([&](Image *p){
		if (p->image_.format() == QImage::Format_Grayscale8) {
			int w = p->image_.width();
			int h = p->image_.height();
			int x0 = w;
			int y0 = h;
			int x1 = 0;
			int y1 = 0;
			for (int y = 0; y < h; y++) {
				uint8_t const *s = p->image_.scanLine(y);
				for (int x = 0; x < w; x++) {
					if (s[x] != 0) {
						x0 = std::min(x0, x);
						y0 = std::min(y0, y);
						x1 = std::max(x1, x);
						y1 = std::max(y1, y);
					}
				}
			}
			if (x0 < x1 && y0 < y1) {
				QRect r = QRect(x0, y0, x1 - x0, y1 - y0).translated(offset() + p->offset());
				if (rect.isNull()) {
					rect = r;
				} else {
					rect = rect.united(r);
				}
			}
		} else if (p->image_.format() == QImage::Format_RGBA8888) {
			int w = p->image_.width();
			int h = p->image_.height();
			int x0 = w;
			int y0 = h;
			int x1 = 0;
			int y1 = 0;
			for (int y = 0; y < h; y++) {
				euclase::PixelRGBA const *s = (euclase::PixelRGBA const *)p->image_.scanLine(y);
				for (int x = 0; x < w; x++) {
					if (s[x].a != 0) {
						x0 = std::min(x0, x);
						y0 = std::min(y0, y);
						x1 = std::max(x1, x);
						y1 = std::max(y1, y);
					}
				}
			}
			if (x0 < x1 && y0 < y1) {
				QRect r = QRect(x0, y0, x1 - x0, y1 - y0).translated(offset() + p->offset());
				if (rect.isNull()) {
					rect = r;
				} else {
					rect = rect.united(r);
				}
			}
		}
	});
	return rect;
}

void Document::changeSelection(SelectionOperation op, const QRect &rect, QMutex *sync)
{
	Document::Layer layer(0, 0);
	auto panel = layer.addPanel();
	panel->offset_ = rect.topLeft();
	panel->image_ = QImage(rect.size(), QImage::Format_Grayscale8);
	panel->image_.fill(Qt::white);
	if (1) {
		panel->image_.fill(Qt::black);
		QPainter pr(&panel->image_);
		pr.setBrush(Qt::white);
		pr.drawEllipse(0, 0, rect.width() - 1, rect.height() - 1);
	}

	switch (op) {
	case SelectionOperation::SetSelection:
		clearSelection(sync);
		addSelection(layer, sync, nullptr);
		break;
	case SelectionOperation::AddSelection:
		addSelection(layer, sync, nullptr);
		break;
	case SelectionOperation::SubSelection:
		subSelection(layer, sync, nullptr);
		break;
	}
}

