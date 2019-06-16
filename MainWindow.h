#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class Document;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT
private:
	struct Private;
	Private *m;
public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

	Document *document();
	Document const *document() const;

	void fitView();
	QImage renderImage(const QRect &r) const;
	QRect selectionRect() const;
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
	void on_horizontalSlider_size_valueChanged(int value);
	void on_horizontalSlider_softness_valueChanged(int value);
	void on_horizontalScrollBar_valueChanged(int value);

	void on_verticalScrollBar_valueChanged(int value);

	void on_action_trim_triggered();

private:
	Ui::MainWindow *ui;
	void setImage(const QImage &image, bool fitview);
	void setImage(QByteArray const &ba);
};

#endif // MAINWINDOW_H
