#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "Document.h"

#include <QMainWindow>

class Brush;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
	Q_OBJECT
private:
	Ui::MainWindow *ui;

	struct Private;
	Private *m;

	void setImage(const QImage &image, bool fitview);
	void setImage(QByteArray const &ba);

	void drawBrush();
	void drawBrush(double x, double y);
	void applyBrush(const Document::Layer &layer, bool update);
protected:
	void keyPressEvent(QKeyEvent *event);
public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

	Document *document();
	Document const *document() const;

	void fitView();
	QImage renderImage(const QRect &r) const;
	QRect selectionRect() const;
	void openFile(const QString &path);
	int documentWidth() const;
	int documentHeight() const;
	QColor foregroundColor() const;
	const Brush &currentBrush() const;
public slots:
	void setForegroundColor(QColor const &color);
	void setCurrentBrush(const Brush &brush);
	void onPenDown(double x, double y);
	void onPenStroke(double x, double y);
	void onPenUp(double x, double y);
	void onMouseLeftButtonPress(int x, int y);
	void onMouseMove(int x, int y, bool leftbutton);
	void onMouseLeftButtonRelase(int x, int y, bool leftbutton);
private slots:
	void onHueChanged(int hue);
	void on_action_file_open_triggered();
	void on_action_file_save_as_triggered();
	void on_action_filter_antialias_triggered();
	void on_action_filter_blur_triggered();
	void on_action_filter_maximize_triggered();
	void on_action_filter_median_triggered();
	void on_action_filter_minimize_triggered();
	void on_action_resize_triggered();
	void on_action_trim_triggered();
	void on_horizontalScrollBar_valueChanged(int value);
	void on_horizontalSlider_size_valueChanged(int value);
	void on_horizontalSlider_softness_valueChanged(int value);
	void on_verticalScrollBar_valueChanged(int value);
};

#endif // MAINWINDOW_H
