
#include "ImageViewWidget.h"
#include "MainWindow.h"
#include "MemoryReader.h"
#include "Photoshop.h"
#include "charvec.h"
#include "joinpath.h"
#include "misc.h"
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
//	FileDiffWidget *filediffwidget = nullptr;
//	FileDiffWidget::DrawData *draw_data = nullptr;
	QScrollBar *v_scroll_bar = nullptr;
	QScrollBar *h_scroll_bar = nullptr;
	QString mime_type;

//	SvgRendererPtr svg;

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
//	bool draw_left_border = true;

	bool left_button = false;

#ifndef APP_GUITAR
	QPixmap transparent_pixmap;
#endif
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
}

ImageViewWidget::~ImageViewWidget()
{
	delete m;
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

void ImageViewWidget::internalScrollImage(double x, double y)
{
	m->image_scroll_x = x;
	m->image_scroll_y = y;
	QSizeF sz = imageScrollRange();
	if (m->image_scroll_x < 0) m->image_scroll_x = 0;
	if (m->image_scroll_y < 0) m->image_scroll_y = 0;
	if (m->image_scroll_x > sz.width()) m->image_scroll_x = sz.width();
	if (m->image_scroll_y > sz.height()) m->image_scroll_y = sz.height();
	update();
}

void ImageViewWidget::scrollImage(double x, double y)
{
	internalScrollImage(x, y);

	if (m->h_scroll_bar) {
		m->h_scroll_bar->blockSignals(true);
		m->h_scroll_bar->setValue((int)m->image_scroll_x);
		m->h_scroll_bar->blockSignals(false);
	}
	if (m->v_scroll_bar) {
		m->v_scroll_bar->blockSignals(true);
		m->v_scroll_bar->setValue((int)m->image_scroll_y);
		m->v_scroll_bar->blockSignals(false);
	}
}

void ImageViewWidget::refrectScrollBar()
{
	double e = 0.75;
	double x = m->h_scroll_bar->value();
	double y = m->v_scroll_bar->value();
	if (fabs(x - m->image_scroll_x) < e) x = m->image_scroll_x; // 差が小さいときは値を維持する
	if (fabs(y - m->image_scroll_y) < e) y = m->image_scroll_y;
	internalScrollImage(x, y);
}

void ImageViewWidget::clear()
{
	m->mime_type = QString();
	document()->image = QImage();
	setMouseTracking(false);
	update();
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
	h->blockSignals(true);
	v->blockSignals(true);
	QSizeF sz = imageScrollRange();
	h->setRange(0, (int)sz.width());
	v->setRange(0, (int)sz.height());
	h->setPageStep(width());
	v->setPageStep(height());
	h->blockSignals(false);
	v->blockSignals(false);
}

void ImageViewWidget::updateScrollBarRange()
{
	setScrollBarRange(m->h_scroll_bar, m->v_scroll_bar);
}

MainWindow *ImageViewWidget::mainwindow()
{
	return m->mainwindow;
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
	return !document()->image.isNull();
}

QSize ImageViewWidget::imageSize() const
{
	if (!document()->image.isNull()) return document()->image.size();
	return QSize();
}

void ImageViewWidget::paintEvent(QPaintEvent *)
{
	QPainter pr(this);

	QSize imagesize = imageSize();
	if (imagesize.width() > 0 && imagesize.height() > 0) {
		pr.save();
		double cx = width() / 2.0;
		double cy = height() / 2.0;
		double x = cx - m->image_scroll_x;
		double y = cy - m->image_scroll_y;
		QSizeF sz = imageScrollRange();
		if (sz.width() > 0 && sz.height() > 0) {
			QBrush br = getTransparentBackgroundBrush();
			pr.setBrushOrigin((int)x, (int)y);
			pr.fillRect((int)x, (int)y, (int)sz.width(), (int)sz.height(), br);
			QImage img = mainwindow()->renderImage(QRect(0, 0, imagesize.width(), imagesize.height()));
			if (!img.isNull()) {
				pr.drawImage(QRect((int)x, (int)y, (int)sz.width(), (int)sz.height()), img, img.rect());
			}
		}
		misc::drawFrame(&pr, (int)x - 1, (int)y - 1, (int)sz.width() + 2, (int)sz.height() + 2, Qt::black);
		pr.restore();
	}
}

void ImageViewWidget::resizeEvent(QResizeEvent *)
{
	updateScrollBarRange();
}

class ImageYUVA64 {
public:
	struct Pixel {
		int16_t y, u, v, a;
		Pixel(int16_t y = 0, int16_t u = 0, int16_t v = 0, int16_t a = 0)
			: y(y), u(u), v(v), a(a)
		{
		}
	};
public:
	static const int SCALE = 16;
	int width_ = 0;
	int height_ = 0;
	QByteArray ba_;
	ImageYUVA64() = default;
	ImageYUVA64(int w, int h, QByteArray const &ba = QByteArray())
		: width_(w)
		, height_(h)
		, ba_(ba)
	{
	}
	ImageYUVA64 copy() const
	{
		return ImageYUVA64(width_, height_, ba_);
	}
	Pixel *scanLine(int y)
	{
		return (Pixel *)ba_.data() + width_ * y;
	}
	static ImageYUVA64 fromImage(QImage const &img)
	{
		ImageYUVA64 yuvimg;
		int w = img.width();
		int h = img.height();
		if (w > 0 && h > 0) {
			yuvimg.width_ = w;
			yuvimg.height_ = h;
			yuvimg.ba_.resize(w * h * 8);
			QImage sourceimg = img.convertToFormat(QImage::Format_RGBA8888);
			const int amp = 256 * SCALE - 1;
			for (int y = 0; y < h; y++) {
				uint8_t const *s = (uint8_t const *)sourceimg.scanLine(y);
				Pixel *d = (Pixel *)yuvimg.ba_.data() + w * y;
				for (int x = 0; x < w; x++) {
					int R = s[x * 4 + 0];
					int G = s[x * 4 + 1];
					int B = s[x * 4 + 2];
					int A = s[x * 4 + 3];
					int Y = SCALE * (R * 16829 + G *  33039 + B *  6416) / 65536;
					int U = SCALE * (R * -9714 + G * -19071 + B * 28784) / 65536 + 2048;
					int V = SCALE * (R * 28784 + G * -24103 + B * -4681) / 65536 + 2048;
					d[x].y = Y;
					d[x].u = U;
					d[x].v = V;
					d[x].a = (A * 2 * amp / 255 + 1) / 2;
				}
			}
		}
		return yuvimg;
	}
	QImage toImage()
	{
		QImage newimage;
		int w = width_;
		int h = height_;
		if (w > 0 && h > 0) {
			newimage = QImage(w, h, QImage::Format_RGBA8888);
			const int amp = 256 * SCALE - 1;
			for (int y = 0; y < h; y++) {
				for (int x = 0; x < w; x++) {
					Pixel const *s = (Pixel *)ba_.data() + w * y;
					uint8_t *d = (uint8_t *)newimage.scanLine(y);
					int Y = s[x].y;
					int U = s[x].u;
					int V = s[x].v;
					int A = s[x].a;
					int R = (((Y) * 76309 +                       (V - 2048) * 104597) / 65536 + SCALE / 2) / SCALE;
					int G = (((Y) * 76309 - (U - 2048) *  25675 - (V - 2048) *  53279) / 65536 + SCALE / 2) / SCALE;
					int B = (((Y) * 76309 + (U - 2048) * 132201                      ) / 65536 + SCALE / 2) / SCALE;
					d[x * 4 + 0] = R;
					d[x * 4 + 1] = G;
					d[x * 4 + 2] = B;
					d[x * 4 + 3] = (A * 2 * 255 / amp + 1) / 2;
				}
			}
		}
		return newimage;
	}
	void filter()
	{
		if (width_ < 1 || height_ < 1) return;

	}
};

QImage ImageViewWidget::filter_median_rgba8888(QImage srcimage)
{
	srcimage = srcimage.convertToFormat(QImage::Format_RGBA8888);
	int w = srcimage.width();
	int h = srcimage.height();
	QImage newimage = srcimage.copy();
	for (int y = 1; y + 1 < h; y++) {
		for (int x = 1; x + 1 < w; x++) {
			uint8_t r[9];
			uint8_t g[9];
			uint8_t b[9];
			int n = 0;
			for (int i = 0; i < 3; i++) {
				for (int j = 0; j < 3; j++) {
					uint8_t *s = srcimage.scanLine((y + i) - 1) + ((x + j) - 1) * 4;
					if (s[3] != 0) {
						r[i * 3 + j] = s[0];
						g[i * 3 + j] = s[1];
						b[i * 3 + j] = s[2];
						n++;
					}
				}
			}
			std::sort(r, r + n);
			std::sort(g, g + n);
			std::sort(b, b + n);
			uint8_t *d = newimage.scanLine(y) + x * 4;
			d[0] = r[4];
			d[1] = g[4];
			d[2] = b[4];
		}
	}
	return newimage;

}

QImage ImageViewWidget::filter_median__yuva64(QImage srcimage)
{
	srcimage = srcimage.convertToFormat(QImage::Format_RGBA8888);
	ImageYUVA64 srcimage2 = ImageYUVA64::fromImage(srcimage);
	int w = srcimage.width();
	int h = srcimage.height();
	ImageYUVA64 newimage = srcimage2.copy();
	for (int y = 1; y + 1 < h; y++) {
		for (int x = 1; x + 1 < w; x++) {
			int Y[9];
			int U[9];
			int V[9];
			int n = 0;
			for (int i = 0; i < 3; i++) {
				for (int j = 0; j < 3; j++) {
					ImageYUVA64::Pixel const *s = srcimage2.scanLine((y + i) - 1) + ((x + j) - 1);
					if (s->a != 0) {
						Y[i * 3 + j] = s->y;
						U[i * 3 + j] = s->u;
						V[i * 3 + j] = s->v;
						n++;
					}
				}
			}
			std::sort(Y, Y + n);
			std::sort(U, U + n);
			std::sort(V, V + n);
			ImageYUVA64::Pixel *d = newimage.scanLine(y) + x;
			d->y = Y[4];
			d->u = U[4];
			d->v = V[4];
		}
	}
	return newimage.toImage();
}

void ImageViewWidget::filter_median_rgba8888()
{
	document()->image = filter_median__yuva64(document()->image);
	update();
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

QPointF ImageViewWidget::mapFromViewport(QPointF const &pos)
{
	double cx = width() / 2.0;
	double cy = height() / 2.0;
	double x = (pos.x() - cx + m->image_scroll_x) / m->image_scale;
	double y = (pos.y() - cy + m->image_scroll_y) / m->image_scale;
	return QPointF(x, y);
}

void ImageViewWidget::mouseMoveEvent(QMouseEvent *e)
{
	if (isValidImage()) {
		QPoint pos = mapFromGlobal(QCursor::pos());
		if (m->left_button && hasFocus()) {
			if (0) {
				int delta_x = pos.x() - m->mouse_press_pos.x();
				int delta_y = pos.y() - m->mouse_press_pos.y();
				scrollImage(m->scroll_origin_x - delta_x, m->scroll_origin_y - delta_y);
			} else {
				mainwindow()->onMouseMove(pos.x(), pos.y(), true);
			}
		}
		m->cursor_anchor_pos = mapFromViewport(pos);
		m->wheel_delta = 0;
	}
}

void ImageViewWidget::mouseReleaseEvent(QMouseEvent *e)
{
	if (isValidImage()) {
		QPoint pos = mapFromGlobal(QCursor::pos());
		if (m->left_button && hasFocus()) {
			mainwindow()->onMouseLeftButtonRelase(pos.x(), pos.y(), true);
		}
	}
	m->left_button = false;
}

void ImageViewWidget::setImageScale(double scale)
{
	if (scale < 1 / 32.0) scale = 1 / 32.0;
	if (scale > 32) scale = 32;
	m->image_scale = scale;
}

void ImageViewWidget::updateCursorAnchorPos()
{
	m->cursor_anchor_pos = mapFromViewport(mapFromGlobal(QCursor::pos()));
}

void ImageViewWidget::updateCenterAnchorPos()
{
	m->center_anchor_pos = mapFromViewport(QPointF(width() / 2.0, height() / 2.0));
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

	scrollImage(w * m->image_scale / 2.0, h * m->image_scale / 2.0);
}

void ImageViewWidget::zoomToCursor(double scale)
{
	QPoint pos = mapFromGlobal(QCursor::pos());

	setImageScale(scale);
	updateScrollBarRange();

	double x = m->cursor_anchor_pos.x() * m->image_scale + width() / 2.0 - (pos.x() + 0.5);
	double y = m->cursor_anchor_pos.y() * m->image_scale + height() / 2.0 - (pos.y() + 0.5);
	scrollImage(x, y);

	updateCenterAnchorPos();

	update();
}

void ImageViewWidget::zoomToCenter(double scale)
{
	setImageScale(scale);
	updateScrollBarRange();

	double x = m->center_anchor_pos.x() * m->image_scale;
	double y = m->center_anchor_pos.y() * m->image_scale;
	scrollImage(x, y);

	updateCursorAnchorPos();

	update();
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

void ImageViewWidget::wheelEvent(QWheelEvent *e)
{
	double scale = 1;
	const double mul = 1.189207115; // sqrt(sqrt(2))
#if 0
	m->wheel_delta += e->delta();
	while (m->wheel_delta >= 120) {
		m->wheel_delta -= 120;
		scale *= mul;
	}
	while (m->wheel_delta <= -120) {
		m->wheel_delta += 120;
		scale /= mul;
	}
#else
	double d = e->delta();
//	double t = 1.003;//pow(sqrt(sqrt(2)), 1.0 / 60);
	double t = 1.001;
//	qDebug() << t;
	scale *= pow(t, d);
#endif
	zoomToCursor(m->image_scale * scale);
}



