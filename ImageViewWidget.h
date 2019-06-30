#ifndef IMAGEVIEWWIDGET_H
#define IMAGEVIEWWIDGET_H

#include <QScrollBar>
#include <QWidget>
#include "MainWindow.h"

class Document;

class ImageViewWidget : public QWidget {
	Q_OBJECT
private:
	struct Private;
	Private *m;

	Document *document();
	Document const *document() const;

	bool isValidImage() const;
	QSize imageSize() const;

	QSizeF imageScrollRange() const;
	void internalScrollImage(double x, double y);
	void scrollImage(double x, double y);
	void setImageScale(double scale);
	QBrush getTransparentBackgroundBrush();
	void setScrollBarRange(QScrollBar *h, QScrollBar *v);
	void updateScrollBarRange();
	void zoomToCursor(double scale);
	void zoomToCenter(double scale);
	void updateCursorAnchorPos();
	void updateCenterAnchorPos();
protected:
	MainWindow *mainwindow();
	void resizeEvent(QResizeEvent *) override;
	void paintEvent(QPaintEvent *) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *e);
	void wheelEvent(QWheelEvent *) override;
public:
	explicit ImageViewWidget(QWidget *parent = nullptr);
	~ImageViewWidget() override;

	void bind(MainWindow *m, QScrollBar *vsb, QScrollBar *hsb);

	void clear();

	void refrectScrollBar();

	void scaleFit(double ratio = 1.0);
	void scale100();
	QPointF mapFromViewport(const QPointF &pos);
	void filter_median_rgba8888();
	void zoomIn();
	void zoomOut();
	static QImage filter_median_rgba8888(QImage srcimage);
	static QImage filter_median_yuva64(QImage srcimage);
signals:
	void scrollByWheel(int lines);

	// QWidget interface
protected:
};

#endif // IMAGEVIEWWIDGET_H
