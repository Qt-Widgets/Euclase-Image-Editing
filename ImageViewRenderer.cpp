#include "ImageViewRenderer.h"
#include "MainWindow.h"


ImageViewRenderer::ImageViewRenderer(QObject *parent)
	: QThread(parent)
{
}

void ImageViewRenderer::run()
{
	QImage image = mainwindow_->renderImage(rect_);
	emit done(image);
}

void ImageViewRenderer::request(MainWindow *mw, const QRect &rect)
{
	mainwindow_ = mw;
	rect_ = rect;
	start();
}

