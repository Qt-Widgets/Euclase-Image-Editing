#include "BrushSlider.h"
#include <functional>
#include <QPainter>
#include <QKeyEvent>
#include "misc.h"

BrushSlider::BrushSlider(QWidget *parent)
	: QSlider(parent)
{
	color_ = Qt::white;
	setVisualType(SIZE);
}

BrushSlider::VisualType BrushSlider::visualType() const
{
	return visual_type_;
}

void BrushSlider::setVisualType(VisualType type)
{
	visual_type_ = type;

	int max = (visual_type_ == SIZE) ? 1000 : 100;
	setMaximum(max);

	update();
}

void BrushSlider::setColor(QColor const &color)
{
	color_ = color;
	update();
}

void BrushSlider::mouseDoubleClickEvent(QMouseEvent *e)
{
	int w = slider_rect_.width();
	if (w > 1) {
		double x = (e->pos().x() - slider_rect_.x()) * (maximum() - minimum()) / (w - 1);
		int v = floor(x + 0.5);
		v = misc::clamp(v, minimum(), maximum());
		setValue(v);
	}
}

void BrushSlider::updateGeometry()
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

void BrushSlider::resizeEvent(QResizeEvent *e)
{
	QWidget::resizeEvent(e);
	updateGeometry();
}

void BrushSlider::paintEvent(QPaintEvent *)
{
	if (image_.isNull()) {
		switch (visual_type_) {
		case VisualType::SIZE:
			image_.load(":/image/size.png");
			break;
		case VisualType::SOFTNESS:
			image_.load(":/image/softness.png");
			break;
		}
	}


	updateGeometry();
	int val = value();
	int max = maximum();

	int w = slider_rect_.width();
	QPainter pr(this);
	pr.fillRect(slider_rect_.adjusted(-1, -1, 1, 1), Qt::black);
	{
		QImage img = image_.scaled(slider_rect_.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
		pr.drawImage(slider_rect_.topLeft(), img);
	}
	pr.setRenderHint(QPainter::Antialiasing);
	{
		QPainterPath path;
		path.addRect(rect());
		QPainterPath path2;
		path2.addEllipse(handle_rect_.adjusted(4, 4, -4, -4));
		path = path.subtracted(path2);
		pr.setClipPath(path);
	}
	pr.setPen(Qt::NoPen);
	pr.setBrush(Qt::black);
	pr.drawEllipse(handle_rect_);
	pr.setBrush(Qt::white);
	pr.drawEllipse(handle_rect_.adjusted(1, 1, -1, -1));
	pr.setPen(Qt::NoPen);
	pr.setBrush(Qt::black);
	pr.drawEllipse(handle_rect_.adjusted(3, 3, -3, -3));
}

void BrushSlider::offset(int delta)
{
	setValue(value() + delta);
}

void BrushSlider::keyPressEvent(QKeyEvent *e)
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

void BrushSlider::mousePressEvent(QMouseEvent *e)
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

void BrushSlider::mouseMoveEvent(QMouseEvent *e)
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

