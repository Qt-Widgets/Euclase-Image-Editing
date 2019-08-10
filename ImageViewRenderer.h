#ifndef IMAGEVIEWRENDERER_H
#define IMAGEVIEWRENDERER_H

#include <QObject>
#include <QRect>
#include <QThread>

class MainWindow;

class ImageViewRenderer : public QThread {
	Q_OBJECT
private:
	volatile bool requested_ = false;
	MainWindow *mainwindow_;
	QRect rect_;
	bool abort_ = false;
protected:
	void run();
public:
	explicit ImageViewRenderer(QObject *parent = nullptr);
	~ImageViewRenderer();
	void request(MainWindow *mw, QRect const &rect);
	void abort();
signals:
	void done(QImage const &image);
};

#endif // IMAGEVIEWRENDERER_H
