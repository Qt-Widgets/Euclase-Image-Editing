#include "ImageViewRenderer.h"
#include "MainWindow.h"


ImageViewRenderer::ImageViewRenderer(QObject *parent)
	: QThread(parent)
{
}

void ImageViewRenderer::run()
{
	while (requested_) {
		requested_ = false;
		QImage image = mainwindow_->renderImage(rect_, false);
		emit done(image);
	}
}

void ImageViewRenderer::request(MainWindow *mw, const QRect &rect)
{
	mainwindow_ = mw;
	rect_ = rect;
	requested_ = true;
	if (!isRunning()) {
		start();
	}
}

