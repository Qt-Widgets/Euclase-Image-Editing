#include "MyApplication.h"
#include "MyWidget.h"

#include <QDebug>
#include <QFile>
#include <QPainter>
#include <QRgb>
#include <QTime>
#include <stdint.h>
#include "main.h"

#if USE_OPENCL
MiraCL *MyWidget::getCL()
{
	return theApp->getCL();
}
#endif

MyWidget::MyWidget(QWidget *parent)
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
	prog.build(getCL(), source, "example");
#endif
}

MyWidget::~MyWidget()
{
#if USE_OPENCL
	prog.release();
#endif
}

void MyWidget::paintEvent(QPaintEvent *)
{
#if USE_OPENCL
	cl_uint w = width();
	cl_uint h = height();

	MiraCL::Buffer buff(getCL());
	buff.alloc(w * h * sizeof(uint32_t));

	if (image.width() != w || image.height() != h) {
		image = QImage(w, h, QImage::Format_ARGB32);
	}

	QTime time;
	time.start();
	prog.arg(0, &w);
	prog.arg(1, &h);
	prog.arg(2, &buff);
	prog.run(w, h);
	buff.read(0, w * h * sizeof(uint32_t), image.bits());
	getCL()->flush();
	getCL()->finish();
	QPainter pr(this);
	pr.drawImage(0, 0, image);
	int ms = time.elapsed();
	qDebug() << ms << "ms";
#endif
}

void MyWidget::resizeEvent(QResizeEvent *)
{
	image = QImage();
}
