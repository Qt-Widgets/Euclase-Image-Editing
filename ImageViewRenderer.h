#ifndef IMAGEVIEWRENDERER_H
#define IMAGEVIEWRENDERER_H

#include <QObject>
#include <QRect>
#include <QThread>

class MainWindow;

class ImageViewRenderer : public QThread {
	Q_OBJECT
private:
	MainWindow *mainwindow_;
	QRect rect_;
public:
	explicit ImageViewRenderer(QObject *parent = nullptr);
	void request(MainWindow *mw, QRect const &rect);
protected:
	void run();
signals:
	void done(QImage const &image);
};

#endif // IMAGEVIEWRENDERER_H
