#ifndef HUEWIDGET_H
#define HUEWIDGET_H

#include <QWidget>

class HueWidget : public QWidget
{
	Q_OBJECT
private:
	int hue;
	int hue_add;
	int press_pos;
	QImage image;
	QPixmap pixmap;
	void emit_hueChanged_();
public:
	explicit HueWidget(QWidget *parent = 0);

signals:

public slots:

	// QWidget interface
protected:
	void paintEvent(QPaintEvent *);

	// QWidget interface
protected:
	void mousePressEvent(QMouseEvent *);
	void mouseMoveEvent(QMouseEvent *);
	void mouseReleaseEvent(QMouseEvent *);

	// QWidget interface
protected:
	void wheelEvent(QWheelEvent *);

	// QWidget interface
protected:
	void mouseDoubleClickEvent(QMouseEvent *);
signals:
	void hueChanged(int hue);
};

#endif // HUEWIDGET_H
