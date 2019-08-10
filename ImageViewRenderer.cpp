#include "ImageViewRenderer.h"
#include "MainWindow.h"


ImageViewRenderer::ImageViewRenderer(QObject *parent)
	: QThread(parent)
{
}

ImageViewRenderer::~ImageViewRenderer()
{
	abort();
}

void ImageViewRenderer::run()
{
	while (requested_) {
		requested_ = false;
		bool quickmask = true;
		QImage image = mainwindow_->renderImage(rect_, quickmask);
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

void ImageViewRenderer::abort()
{
	auto *sync = mainwindow_->synchronizer();
	sync->abort = true;
}

