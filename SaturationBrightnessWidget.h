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
	int hue = 0;
	QPixmap pixmap;
	MainWindow *mainwindow();
	void updatePixmap(bool force);
	QPixmap createPixmap(int w, int h);
	void changeColor(const QColor &color);
public:
	explicit SaturationBrightnessWidget(QWidget *parent = 0);

	void setHue(int h);
signals:

public slots:

	// QWidget interface
protected:
	void paintEvent(QPaintEvent *);
};

#endif // SATURATIONBRIGHTNESSWIDGET_H
