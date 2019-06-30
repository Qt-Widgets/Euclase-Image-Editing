#include "BrushPreviewWidget.h"
#include "MyApplication.h"
#include <QDebug>
#include <QFile>
#include <QImage>
#include <QPainter>
#include <QTime>
#include <math.h>
#include "RoundBrushGenerator.h"

MainWindow *BrushPreviewWidget::mainwindow()
{
	return qobject_cast<MainWindow *>(window());
}

BrushPreviewWidget::BrushPreviewWidget(QWidget *parent)
	: QWidget(parent)
{
	QString path = ":/cl/example.cl";
	std::string source;
	QFile file(path);
	if (file.open(QFile::ReadOnly)) {
		QByteArray ba = file.readAll();
		if (!ba.isEmpty()) {
			source = std::string(ba.begin(), ba.end());
		}
	}

#if USE_OPENCL
	prog.build(getCL(), source, "round_brush_level");
#endif
}

void BrushPreviewWidget::paintEvent(QPaintEvent *)
{
	int w = width();
	int h = height();
	int cx = w / 2;
	int cy = h / 2;
	double x = cx + 0.5;
	double y = cy + 0.5;

	RoundBrushGenerator brush(brush_.size, brush_.softness);

	QImage image(w, h, QImage::Format_ARGB32);
#if USE_OPENCL
	QTime time;
	time.start();
	MiraCL::Buffer buff(getCL());
	buff.alloc(w * h * sizeof(cl_float));
	prog.arg(0, &w);
	prog.arg(1, &h);
	prog.arg(2, &cx);
	prog.arg(3, &cy);
	prog.arg(4, &brush.radius);
	prog.arg(5, &brush.blur);
	prog.arg(6, &brush.mul);
	prog.arg(7, &buff);
	prog.run(w, h);
	std::vector<cl_float> floatbuffer(w * h);
	buff.read(0, w * h * sizeof(cl_float), &floatbuffer[0]);
	for (int i = 0; i < h; i++) {
		for (int j = 0; j < w; j++) {
			QRgb *dst = reinterpret_cast<QRgb *>(image.scanLine(i));
			float value = floatbuffer[i * w + j];
			int v = (int)(value  * 255);
			dst[j] = qRgb(v, v, v);
		}
	}
	int ms = time.elapsed();
	qDebug() << ms << "ms";
#else
	for (int i = 0; i < h; i++) {
		for (int j = 0; j < w; j++) {
			QRgb *dst = reinterpret_cast<QRgb *>(image.scanLine(i));
			double tx = j + 0.5;
			double ty = i + 0.5;
			double value = brush.level(tx - x, ty - y);
			int v = (int)(value  * 255);
			dst[j] = qRgb(v, v, v);
		}
	}
#endif

	QPainter pr(this);
	pr.drawImage(0, 0, image);
}

void BrushPreviewWidget::changeBrush()
{
	mainwindow()->setCurrentBrush(brush_);
	update();
}

double BrushPreviewWidget::brushSize() const
{
	return brush_.size;
}

double BrushPreviewWidget::brushSoftness() const
{
	return brush_.softness;
}

void BrushPreviewWidget::setBrushSize(double v)
{
	brush_.size = v;
	changeBrush();
}

void BrushPreviewWidget::setBrushSoftness(double v)
{
	brush_.softness = v;
	changeBrush();
}

void BrushPreviewWidget::setBrush(Brush const &b)
{
	brush_ = b;
	changeBrush();
}

#if USE_OPENCL
MiraCL *BrushPreviewWidget::getCL()
{
	return theApp->getCL();
}
#endif

float approx_distance(float x, float y)
{
	float n = x * x + y * y;

	float s = 1;
	s = (n / s + s) / 2;
	s = (n / s + s) / 2;
	s = (n / s + s) / 2;
	s = (n / s + s) / 2;
	return s;
}


