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
	int w = std::max(0.0, width() / scale_);
	int h = std::max(0.0, height() / scale_);
	double cx = (w / 2) + 1.5;
	double cy = (h / 2) + 1.5;
	w += 2;
	h += 2;

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
			double value = brush.level(tx - cx, ty - cy);
			int v = (int)(value * 255);
			dst[j] = qRgb(v, v, v);
		}
	}
#endif

	QPainter pr(this);
	int x = width() / 2 - cx * scale_;
	int y = height() / 2 - cy * scale_;
	pr.fillRect(0, 0, width(), height(), Qt::black);
	pr.drawImage(QRect(x, y, w * scale_, h * scale_), image);
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
	setBrush_(b);
	changeBrush();
}

void BrushPreviewWidget::setBrush_(Brush const &b)
{
	brush_ = b;
	update();
}

#if USE_OPENCL
MiraCL *BrushPreviewWidget::getCL()
{
	return theApp->getCL();
}
#endif

void BrushPreviewWidget::changeScale(double scale)
{
	scale_ = scale;
	update();
}



