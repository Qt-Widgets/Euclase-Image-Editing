
#include "ImageViewWidget.h"
#include "Document.h"
#include "ImageViewRenderer.h"
#include "ImageViewWidget.h"
#include "MainWindow.h"
#include "MemoryReader.h"
#include "Photoshop.h"
#include "SelectionOutlineRenderer.h"
#include "charvec.h"
#include "joinpath.h"
#include "misc.h"
#include <QBitmap>
#include <QBuffer>
#include <QDebug>
#include <QFileDialog>
#include <QMenu>
#include <QPainter>
#include <QSvgRenderer>
#include <QWheelEvent>
#include <cmath>
#include <functional>
#include <memory>

using SvgRendererPtr = std::shared_ptr<QSvgRenderer>;

struct ImageViewWidget::Private {
	MainWindow *mainwindow = nullptr;
	QScrollBar *v_scroll_bar = nullptr;
	QScrollBar *h_scroll_bar = nullptr;
	QString mime_type;
	Synchronize sync;

	double image_scroll_x = 0;
	double image_scroll_y = 0;
	double image_scale = 1;
	double scroll_origin_x = 0;
	double scroll_origin_y = 0;
	QPoint mouse_press_pos;
	int wheel_delta = 0;
	QPointF cursor_anchor_pos;
	QPointF center_anchor_pos;
	int top_margin = 1;
	int bottom_margin = 1;

	bool left_button = false;

	ImageViewRenderer *renderer = nullptr;
	QImage rendered_image;
	QRect destination_rect;

	SelectionOutlineRenderer *outline_renderer = nullptr;

	QPixmap transparent_pixmap;

	int stripe_animation = 0;

	bool rect_visible = false;
	QPointF rect_start;
	QPointF rect_end;

	SelectionOutlineBitmap selection_outline;

};

ImageViewWidget::ImageViewWidget(QWidget *parent)
	: QWidget(parent)
	, m(new Private)
{
#if defined(Q_OS_WIN32)
	setFont(QFont("MS Gothic"));
#elif defined(Q_OS_LINUX)
	setFont(QFont("Monospace"));
#elif defined(Q_OS_MAC)
	setFont(QFont("Menlo"));
#endif

	setContextMenuPolicy(Qt::DefaultContextMenu);

	m->renderer = new ImageViewRenderer(this);
	connect(m->renderer, &ImageViewRenderer::done, this, &ImageViewWidget::onRenderingCompleted);

	m->outline_renderer = new SelectionOutlineRenderer(this);
	connect(m->outline_renderer, &SelectionOutlineRenderer::done, this, &ImageViewWidget::onSelectionOutlineRenderingCompleted);

	startTimer(100);
}

ImageViewWidget::~ImageViewWidget()
{
	delete m;
}

MainWindow *ImageViewWidget::mainwindow()
{
	return m->mainwindow;
}

Document *ImageViewWidget::document()
{
	return m->mainwindow->document();
}

Document const *ImageViewWidget::document() const
{
	return m->mainwindow->document();
}

void ImageViewWidget::bind(MainWindow *mainwindow, QScrollBar *vsb, QScrollBar *hsb)
{
	m->mainwindow = mainwindow;
	m->v_scroll_bar = vsb;
	m->h_scroll_bar = hsb;
}

QPointF ImageViewWidget::mapFromViewportToDocument(QPointF const &pos)
{
	double cx = width() / 2.0;
	double cy = height() / 2.0;
	double x = (pos.x() - cx + m->image_scroll_x) / m->image_scale;
	double y = (pos.y() - cy + m->image_scroll_y) / m->image_scale;
	return QPointF(x, y);
}

QPointF ImageViewWidget::mapFromDocumentToViewport(QPointF const &pos)
{
	double cx = width() / 2.0;
	double cy = height() / 2.0;
	double x = pos.x() * m->image_scale + cx - m->image_scroll_x;
	double y = pos.y() * m->image_scale + cy - m->image_scroll_y;
	return QPointF(x, y);
}

Synchronize *ImageViewWidget::synchronizer()
{
	return &m->sync;
}

void ImageViewWidget::showRect(QPointF const &start, QPointF const &end)
{
	m->rect_start = start;
	m->rect_end = end;
	m->rect_visible = true;
	update();
}

void ImageViewWidget::hideRect()
{
	m->rect_visible = false;
	update();
}

QBrush ImageViewWidget::stripeBrush(bool blink)
{
	int mask = blink ? 2 : 4;
	int a = m->stripe_animation;
	QImage image(8, 8, QImage::Format_Indexed8);
	image.setColor(0, qRgb(0, 0, 0));
	image.setColor(1, qRgb(255, 255, 255));
	if (blink) {
		uint8_t v = (a & 4) ? 1: 0;
		for (int y = 0; y < 8; y++) {
			uint8_t *p = image.scanLine(y);
			for (int x = 0; x < 8; x++) {
				p[x] = v;
			}
		}
	} else {
		for (int y = 0; y < 8; y++) {
			uint8_t *p = image.scanLine(y);
			for (int x = 0; x < 8; x++) {
				p[x] = ((a - x - y) & mask) ? 1 : 0;
			}
		}
	}
	return QBrush(image);
}

void ImageViewWidget::internalScrollImage(double x, double y, bool updateview)
{
	m->image_scroll_x = x;
	m->image_scroll_y = y;
	QSizeF sz = imageScrollRange();
	if (m->image_scroll_x < 0) m->image_scroll_x = 0;
	if (m->image_scroll_y < 0) m->image_scroll_y = 0;
	if (m->image_scroll_x > sz.width()) m->image_scroll_x = sz.width();
	if (m->image_scroll_y > sz.height()) m->image_scroll_y = sz.height();

	if (updateview) {
		paintViewLater(true, true);
	}
}

void ImageViewWidget::scrollImage(double x, double y, bool updateview)
{
	internalScrollImage(x, y, updateview);

	if (m->h_scroll_bar) {
		auto b = m->h_scroll_bar->blockSignals(true);
		m->h_scroll_bar->setValue((int)m->image_scroll_x);
		m->h_scroll_bar->blockSignals(b);
	}
	if (m->v_scroll_bar) {
		auto b = m->v_scroll_bar->blockSignals(true);
		m->v_scroll_bar->setValue((int)m->image_scroll_y);
		m->v_scroll_bar->blockSignals(b);
	}
}

void ImageViewWidget::refrectScrollBar()
{
	double e = 0.75;
	double x = m->h_scroll_bar->value();
	double y = m->v_scroll_bar->value();
	if (fabs(x - m->image_scroll_x) < e) x = m->image_scroll_x; // 差が小さいときは値を維持する
	if (fabs(y - m->image_scroll_y) < e) y = m->image_scroll_y;
	internalScrollImage(x, y, true);
}

void ImageViewWidget::clear()
{
	m->mime_type = QString();
	document()->current_layer()->image() = QImage();
	setMouseTracking(false);
	paintViewLater(true, true);
}

QSizeF ImageViewWidget::imageScrollRange() const
{
	QSize sz = imageSize();
	int w = int(sz.width() * m->image_scale);
	int h = int(sz.height() * m->image_scale);
	return QSize(w, h);
}

void ImageViewWidget::setScrollBarRange(QScrollBar *h, QScrollBar *v)
{
	auto bh = h->blockSignals(true);
	auto bv = v->blockSignals(true);
	QSizeF sz = imageScrollRange();
	h->setRange(0, (int)sz.width());
	v->setRange(0, (int)sz.height());
	h->setPageStep(width());
	v->setPageStep(height());
	h->blockSignals(bh);
	v->blockSignals(bv);
}

void ImageViewWidget::updateScrollBarRange()
{
	setScrollBarRange(m->h_scroll_bar, m->v_scroll_bar);
}

QBrush ImageViewWidget::getTransparentBackgroundBrush()
{
	if (m->transparent_pixmap.isNull()) {
		m->transparent_pixmap = QPixmap(":/image/transparent.png");
	}
	return m->transparent_pixmap;
}

bool ImageViewWidget::isValidImage() const
{
	return !document()->current_layer()->image().isNull();
}

QSize ImageViewWidget::imageSize() const
{
	auto layer = document()->current_layer();
	return layer ? layer->size() : QSize();
}

void ImageViewWidget::setSelectionOutline(const SelectionOutlineBitmap &data)
{
	m->selection_outline = data;
}

void ImageViewWidget::clearSelectionOutline()
{
	m->outline_renderer->abort();
	setSelectionOutline(SelectionOutlineBitmap());
}

void ImageViewWidget::onRenderingCompleted(QImage const &image)
{
	m->rendered_image = image;
	update();
}

void ImageViewWidget::onSelectionOutlineRenderingCompleted(SelectionOutlineBitmap const &data)
{
	setSelectionOutline(data);
	update();
}

void ImageViewWidget::calcDestinationRect()
{
	double cx = width() / 2.0;
	double cy = height() / 2.0;
	double x = cx - m->image_scroll_x;
	double y = cy - m->image_scroll_y;
	QSizeF sz = imageScrollRange();
	m->destination_rect = QRect((int)x, (int)y, (int)sz.width(), (int)sz.height());
}

void ImageViewWidget::paintViewLater(bool image, bool selection_outline)
{
	calcDestinationRect();

	QSize imagesize = imageSize();

	if (image) {
		m->renderer->request(mainwindow(), QRect(0, 0, imagesize.width(), imagesize.height()));
	}

	if (selection_outline) {
		clearSelectionOutline();
		m->outline_renderer->request(mainwindow(), QRect(0, 0, imagesize.width(), imagesize.height()));
	}
}

void ImageViewWidget::updateCursorAnchorPos()
{
	m->cursor_anchor_pos = mapFromViewportToDocument(mapFromGlobal(QCursor::pos()));
}

void ImageViewWidget::updateCenterAnchorPos()
{
	m->center_anchor_pos = mapFromViewportToDocument(QPointF(width() / 2.0, height() / 2.0));
}

void ImageViewWidget::setImageScale(double scale, bool updateview)
{
	if (scale < 1 / 32.0) scale = 1 / 32.0;
	if (scale > 32) scale = 32;
	m->image_scale = scale;

	if (updateview) {
		paintViewLater(true, true);
	}
}

void ImageViewWidget::scaleFit(double ratio)
{
	QSize sz = imageSize();
	double w = sz.width();
	double h = sz.height();
	if (w > 0 && h > 0) {
		double sx = width() / w;
		double sy = height() / h;
		m->image_scale = (sx < sy ? sx : sy) * ratio;
	}
	updateScrollBarRange();

	updateCursorAnchorPos();

	scrollImage(w * m->image_scale / 2.0, h * m->image_scale / 2.0, true);
}

void ImageViewWidget::zoomToCursor(double scale)
{
	clearSelectionOutline();

	QPoint pos = mapFromGlobal(QCursor::pos());

	setImageScale(scale, false);
	updateScrollBarRange();

	double x = m->cursor_anchor_pos.x() * m->image_scale + width() / 2.0 - (pos.x() + 0.5);
	double y = m->cursor_anchor_pos.y() * m->image_scale + height() / 2.0 - (pos.y() + 0.5);
	scrollImage(x, y, true);

	updateCenterAnchorPos();
}

void ImageViewWidget::zoomToCenter(double scale)
{
	setImageScale(scale, false);
	updateScrollBarRange();

	double x = m->center_anchor_pos.x() * m->image_scale;
	double y = m->center_anchor_pos.y() * m->image_scale;
	scrollImage(x, y, true);

	updateCursorAnchorPos();
}

void ImageViewWidget::scale100()
{
	zoomToCenter(1.0);
}

void ImageViewWidget::zoomIn()
{
	zoomToCenter(m->image_scale * 2);
}

void ImageViewWidget::zoomOut()
{
	zoomToCenter(m->image_scale / 2);
}

SelectionOutlineBitmap ImageViewWidget::renderSelectionOutlineBitmap(bool *abort)
{
	SelectionOutlineBitmap data;
	int dw = document()->width();
	int dh = document()->height();
	if (dw > 0 && dh > 0) {
		QPointF vp0(0, 0);
		QPointF vp1(dw, dh);
		vp0 = mapFromDocumentToViewport(vp0);
		vp1 = mapFromDocumentToViewport(vp1);
		vp0.rx() = std::max(vp0.rx(), (double)0);
		vp0.ry() = std::max(vp0.ry(), (double)0);
		vp1.rx() = std::min(vp1.rx(), (double)width());
		vp1.ry() = std::min(vp1.ry(), (double)height());
		int vw = vp1.x() - vp0.x();
		int vh = vp1.y() - vp0.y();
		QPointF dp0 = mapFromViewportToDocument(vp0);
		QPointF dp1 = mapFromViewportToDocument(vp1);
		QImage selection;
		{
			int x = floor(dp0.x());
			int y = floor(dp0.y());
			int w = floor(dp1.x()) - x;
			int h = floor(dp1.y()) - y;
			selection = document()->renderSelection(QRect(x, y, w, h), &m->sync, abort);
			if (abort && *abort) return SelectionOutlineBitmap();
			selection = selection.scaled(vw, vh);
			data.point = mapFromDocumentToViewport(QPointF(x, y)).toPoint();
		}
		if (selection.width() > 0 && selection.height() > 0) {
			QImage image(vw, vh, QImage::Format_Grayscale8);
			image.fill(Qt::white);
			for (int y = 1; y + 1 < vh; y++) {
				if (abort && *abort) return SelectionOutlineBitmap();
				uint8_t const *s0 = selection.scanLine(y - 1);
				uint8_t const *s1 = selection.scanLine(y);
				uint8_t const *s2 = selection.scanLine(y + 1);
				uint8_t *d = image.scanLine(y);
				for (int x = 1; x + 1 < vw; x++) {
					uint8_t v = ~(s0[x - 1] & s0[x] & s0[x + 1] & s1[x - 1] & s1[x + 1] & s2[x - 1] & s2[x] & s2[x + 1]) & s1[x];
					d[x] = (v & 0x80) ? 0 : 255;
				}
			}
			data.bitmap = QBitmap::fromImage(image);
		}
	}
	return data;
}

void ImageViewWidget::paintEvent(QPaintEvent *)
{
	QPainter pr(this);
	int x = m->destination_rect.x();
	int y = m->destination_rect.y();
	int w = m->destination_rect.width();
	int h = m->destination_rect.height();
	if (w > 0 && h > 0) {
		if (!m->rendered_image.isNull()) {
			pr.drawImage(m->destination_rect, m->rendered_image, m->rendered_image.rect());
		}
		misc::drawFrame(&pr, (int)x - 1, (int)y - 1, (int)w + 2, (int)h + 2, Qt::black, Qt::black);
	}

	if (!m->selection_outline.bitmap.isNull()) {
		QBrush brush = stripeBrush(false);
		pr.save();
		pr.setClipRegion(QRegion(m->selection_outline.bitmap).translated(m->selection_outline.point));
		pr.setOpacity(0.5);
		pr.fillRect(0, 0, width(), height(), brush);
		pr.restore();
	}

	if (m->rect_visible) {
		pr.setOpacity(0.5);
		QBrush brush = stripeBrush(true);
		double x0 = floor(m->rect_start.x());
		double y0 = floor(m->rect_start.y());
		double x1 = floor(m->rect_end.x());
		double y1 = floor(m->rect_end.y());
		if (x0 > x1) std::swap(x0, x1);
		if (y0 > y1) std::swap(y0, y1);
		QPointF pt;
		pt = mapFromDocumentToViewport(QPointF(x0, y0));
		x0 = floor(pt.x());
		y0 = floor(pt.y());
		pt = mapFromDocumentToViewport(QPointF(x1 + 1, y1 + 1));
		x1 = floor(pt.x());
		y1 = floor(pt.y());
		misc::drawFrame(&pr, x0, y0, x1 - x0, y1 - y0, brush, brush);
	}
}

void ImageViewWidget::resizeEvent(QResizeEvent *)
{
	clearSelectionOutline();
	updateScrollBarRange();
	paintViewLater(true, true);
}

void ImageViewWidget::mousePressEvent(QMouseEvent *e)
{
	m->left_button = (e->buttons() & Qt::LeftButton);
	if (m->left_button) {
		QPoint pos = mapFromGlobal(QCursor::pos());
		m->mouse_press_pos = pos;
		m->scroll_origin_x = m->image_scroll_x;
		m->scroll_origin_y = m->image_scroll_y;
		mainwindow()->onMouseLeftButtonPress(pos.x(), pos.y());
	}
}

void ImageViewWidget::mouseMoveEvent(QMouseEvent *)
{
	if (isValidImage()) {
		QPoint pos = mapFromGlobal(QCursor::pos());
		if (m->left_button && hasFocus()) {
			if (!mainwindow()->onMouseMove(pos.x(), pos.y(), true)) {
				clearSelectionOutline();
				int delta_x = pos.x() - m->mouse_press_pos.x();
				int delta_y = pos.y() - m->mouse_press_pos.y();
				scrollImage(m->scroll_origin_x - delta_x, m->scroll_origin_y - delta_y, true);
			}
		}
		m->cursor_anchor_pos = mapFromViewportToDocument(pos);
		m->wheel_delta = 0;
	}
}

void ImageViewWidget::mouseReleaseEvent(QMouseEvent *)
{
	if (isValidImage()) {
		QPoint pos = mapFromGlobal(QCursor::pos());
		if (m->left_button && hasFocus()) {
			mainwindow()->onMouseLeftButtonRelase(pos.x(), pos.y(), true);
		}
	}
	m->left_button = false;
}

void ImageViewWidget::wheelEvent(QWheelEvent *e)
{
	double scale = 1;
	double d = e->delta();
	double t = 1.001;
	scale *= pow(t, d);
	zoomToCursor(m->image_scale * scale);
}

void ImageViewWidget::timerEvent(QTimerEvent *)
{
	m->stripe_animation = (m->stripe_animation + 1) & 7;
	update();
}

