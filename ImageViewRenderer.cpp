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
		bool quickmask = false;
		RenderedImage ri;
		ri.rect = rect_;
		ri.image = mainwindow_->renderImage(rect_, quickmask, &abort_);
		emit done(ri);
	}
}

void ImageViewRenderer::request(MainWindow *mw, const QRect &rect)
{
	mainwindow_ = mw;
	rect_ = rect;
	requested_ = true;
	abort_ = false;
	if (!isRunning()) {
		start();
	}
}

void ImageViewRenderer::abort()
{
	abort_ = true;
	wait();
}

