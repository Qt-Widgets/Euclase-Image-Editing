#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

	void fit();
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
private:
	Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
