#ifndef COLORSLIDER_H
#define COLORSLIDER_H

#include <QSlider>

class ColorSlider : public QSlider {
	Q_OBJECT
public:
	enum ColorType {
		RGB_R,
		RGB_G,
		RGB_B,
		HSV_H,
		HSV_S,
		HSV_V,
	};
private:
	QColor color_;
	ColorType color_type_ = HSV_H;
	int handle_size_ = 16;
	QRect slider_rect_;
	QRect handle_rect_;
	int mouse_press_value_;
	QPoint mouse_press_pos_;
	void updateGeometry();
	void offset(int delta);
protected:
	void resizeEvent(QResizeEvent *e);
	void paintEvent(QPaintEvent *);
	void keyPressEvent(QKeyEvent *);
	void mousePressEvent(QMouseEvent *e);
	void mouseMoveEvent(QMouseEvent *e);
public:
	explicit ColorSlider(QWidget *parent = nullptr);
	ColorType colorType() const;
	void setColorType(ColorType colorType);
	void setColor(const QColor &color);
};

#endif // COLORSLIDER_H
