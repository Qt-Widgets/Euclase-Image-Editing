#include "ImageViewRenderer.h"
#include "MainWindow.h"
#include "TransparentCheckerBrush.h"
#include <QPainter>


ImageViewRenderer::ImageViewRenderer(QObject *parent)
	: QThread(parent)
{
}

ImageViewRenderer::~ImageViewRenderer()
{
	abort(true);
}

void ImageViewRenderer::run()
{
	while (requested_) {
		requested_ = false;
		bool quickmask = false;
		RenderedImage ri;
		ri.rect = rect_;
		ri.image = mainwindow_->renderImage(ri.rect, quickmask, &abort_);
		if (!abort_) {
			emit done(ri);
		}
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

void ImageViewRenderer::abort(bool wait)
{
	abort_ = true;
	if (wait) {
		QThread::wait();
	}
}

