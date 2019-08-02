#include "ColorSlider.h"
#include <functional>
#include <QPainter>
#include <QKeyEvent>

ColorSlider::ColorSlider(QWidget *parent)
	: QSlider(parent)
{
	color_ = Qt::white;
	setColorType(HSV_H);
}

ColorSlider::ColorType ColorSlider::colorType() const
{
	return color_type_;
}

void ColorSlider::setColorType(ColorType type)
{
	color_type_ = type;

	int max = (color_type_ == HSV_H) ? 359 : 255;
	setMaximum(max);

	update();
}

void ColorSlider::setColor(QColor const &color)
{
	color_ = color;
	update();
}

void ColorSlider::updateGeometry()
{
	handle_size_ = height();

	int x = handle_size_ / 2;
	int w = width() - handle_size_;
	slider_rect_ = QRect(x, 3, w, height() - 6);

	int val = value();
	int max = maximum();
	int handle_x = val * slider_rect_.width() / (max + 1) + slider_rect_.x() - handle_size_ / 2;
	handle_rect_ = QRect(handle_x, 0, handle_size_, handle_size_);
}

void ColorSlider::resizeEvent(QResizeEvent *e)
{
	QWidget::resizeEvent(e);
	updateGeometry();
}

void ColorSlider::paintEvent(QPaintEvent *)
{
	updateGeometry();
	int val = value();
	int max = maximum();

	std::function<QColor(int t)> color;

	auto rgb_r = [&](int r){ return QColor(r, color_.green(), color_.blue()); };
	auto rgb_g = [&](int g){ return QColor(color_.red(), g, color_.blue()); };
	auto rgb_b = [&](int b){ return QColor(color_.red(), color_.green(), b); };
	auto hsv_h = [&](int h){ return QColor::fromHsv(h, 255, 255); };
	auto hsv_s = [&](int s){ return QColor::fromHsv(color_.hue(), s, color_.value()); };
	auto hsv_v = [&](int v){ return QColor::fromHsv(color_.hue(), color_.saturation(), v); };
	auto unknown = [&](int i){ return QColor(i, i, i); };

	switch (color_type_) {
	case RGB_R: color = rgb_r; break;
	case RGB_G: color = rgb_g; break;
	case RGB_B: color = rgb_b; break;
	case HSV_H: color = hsv_h; break;
	case HSV_S: color = hsv_s; break;
	case HSV_V: color = hsv_v; break;
	default: color = unknown; break;
	}

	int w = slider_rect_.width();
	QPainter pr(this);
	QImage img(w, 1, QImage::Format_ARGB32);
	for (int i = 0; i < w; i++) {
		int t = i * (max + 1) / w;
		reinterpret_cast<QRgb *>(img.scanLine(0))[i] = color(t).rgb();
	}
	pr.fillRect(slider_rect_.adjusted(-1, -1, 1, 1), Qt::black);
	pr.drawImage(slider_rect_, img, img.rect());
	pr.setRenderHint(QPainter::Antialiasing);
	pr.setPen(Qt::NoPen);
	pr.setBrush(Qt::black);
	pr.drawEllipse(handle_rect_);
	pr.setBrush(Qt::white);
	pr.drawEllipse(handle_rect_.adjusted(1, 1, -1, -1));
	pr.setPen(Qt::NoPen);
	pr.setBrush(Qt::black);
	pr.drawEllipse(handle_rect_.adjusted(3, 3, -3, -3));
	pr.setBrush(color(val));
	pr.drawEllipse(handle_rect_.adjusted(4, 4, -4, -4));
}

void ColorSlider::offset(int delta)
{
	setValue(value() + delta);
}

void ColorSlider::keyPressEvent(QKeyEvent *e)
{
	int k = e->key();

	switch (k) {
	case Qt::Key_Home:
		setValue(minimum());
		return;
	case Qt::Key_End:
		setValue(maximum());
		return;
	case Qt::Key_Left:
		offset(-singleStep());
		return;
	case Qt::Key_Right:
		offset(singleStep());
		return;
	case Qt::Key_PageDown:
		offset(-pageStep());
		return;
	case Qt::Key_PageUp:
		offset(pageStep());
		return;
	}
}

void ColorSlider::mousePressEvent(QMouseEvent *e)
{
	int x = e->pos().x();
	if (x < handle_rect_.x()) {
		offset(-pageStep());
		return;
	}
	if (x >= handle_rect_.x() + handle_rect_.width()) {
		offset(pageStep());
		return;
	}

	mouse_press_value_ = value();
	mouse_press_pos_ = e->pos();
}

void ColorSlider::mouseMoveEvent(QMouseEvent *e)
{
	if (e->buttons() & Qt::LeftButton) {
		double slider_w = slider_rect_.width();
		double range = maximum() - minimum();
		double x = (mouse_press_value_ - minimum()) * slider_w / range + e->pos().x() - mouse_press_pos_.x();
		int v = int(x * range / slider_w + minimum());
		setValue(v);
		return;
	}
}

