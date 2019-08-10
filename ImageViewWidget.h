#ifndef IMAGEVIEWWIDGET_H
#define IMAGEVIEWWIDGET_H

#include <QScrollBar>
#include <QWidget>
#include "MainWindow.h"
#include "SelectionOutlineRenderer.h"

class Document;

class ImageViewWidget : public QWidget {
	Q_OBJECT
private:
	struct Private;
	Private *m;

	MainWindow *mainwindow();

	Document *document();
	Document const *document() const;

	bool isValidImage() const;
	QSize imageSize() const;

	QSizeF imageScrollRange() const;
	void internalScrollImage(double x, double y, bool updateview);
	void scrollImage(double x, double y, bool updateview);
	void setImageScale(double scale, bool updateview);
	QBrush getTransparentBackgroundBrush();
	void setScrollBarRange(QScrollBar *h, QScrollBar *v);
	void updateScrollBarRange();
	void zoomToCursor(double scale);
	void zoomToCenter(double scale);
	void updateCursorAnchorPos();
	void updateCenterAnchorPos();
	void calcDestinationRect();
	QBrush stripeBrush(bool blink);
protected:
	void resizeEvent(QResizeEvent *) override;
	void paintEvent(QPaintEvent *) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *e);
	void wheelEvent(QWheelEvent *) override;
public:
	explicit ImageViewWidget(QWidget *parent = nullptr);
	~ImageViewWidget() override;

	void showRect(const QPointF &start, const QPointF &end);
	void hideRect();

	void bind(MainWindow *m, QScrollBar *vsb, QScrollBar *hsb);

	void clear();

	void refrectScrollBar();

	void scaleFit(double ratio = 1.0);
	void scale100();
	QPointF mapToDocument(const QPointF &pos);
	QPointF mapToViewport(QPointF const &pos);
	void zoomIn();
	void zoomOut();

	void paintViewLater(bool image, bool selection_outline);

	void setSelectionOutline(SelectionOutlineBitmap const &data);
	void clearSelectionOutline();
	QBitmap updateSelection_();
	Synchronize *synchronizer();
	SelectionOutlineBitmap renderSelectionOutlineBitmap(bool *abort);
signals:
	void scrollByWheel(int lines);
private slots:
	void onRenderingCompleted(const QImage &image);
	void onSelectionOutlineRenderingCompleted(const SelectionOutlineBitmap &data);
protected:
	void timerEvent(QTimerEvent *event);
};

#endif // IMAGEVIEWWIDGET_H
