#ifndef SATURATIONBRIGHTNESSWIDGET_H
#define SATURATIONBRIGHTNESSWIDGET_H

#include <QWidget>
#include <QPixmap>
#include "MiraCL.h"

class MainWindow;

class SaturationBrightnessWidget : public QWidget {
	Q_OBJECT
private:
#if USE_OPENCL
	MiraCL *getCL();
	MiraCL::Program prog;
#endif
	struct Private;
	Private *m;
	MainWindow *mainwindow();
	void updatePixmap(bool force);
	QImage createImage(int w, int h);
	void press(const QPoint &pos);
protected:
	void paintEvent(QPaintEvent *);
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
public:
	explicit SaturationBrightnessWidget(QWidget *parent = 0);
	~SaturationBrightnessWidget();
	void setHue(int h);
signals:
	void changeColor(const QColor &color);

};

#endif // SATURATIONBRIGHTNESSWIDGET_H
