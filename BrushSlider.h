#ifndef BRUSHSLIDER_H
#define BRUSHSLIDER_H

#include <QSlider>

class BrushSlider : public QSlider {
	Q_OBJECT
public:
	enum VisualType {
		SIZE,
		SOFTNESS,
	};
private:
	QColor color_;
	QImage image_;
	VisualType visual_type_ = SIZE;
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
	explicit BrushSlider(QWidget *parent = nullptr);
	VisualType visualType() const;
	void setVisualType(VisualType visualType);
	void setColor(const QColor &color);

	// QWidget interface
protected:
	void mouseDoubleClickEvent(QMouseEvent *);
};

#endif // BRUSHSLIDER_H
