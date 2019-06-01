#include "MainWindow.h"
#include "ResizeDialog.h"
#include "antialias.h"
#include "median.h"
#include "resize.h"
#include "ui_MainWindow.h"
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	ui->horizontalSlider_size->setValue(1);
	ui->horizontalSlider_softness->setValue(0);
	ui->widget->setBrushSize(1);
	ui->widget->setBrushSoftness(0 / 100.0);

	ui->widget_image_view->bind(this, ui->horizontalScrollBar, ui->verticalScrollBar);

	connect(ui->widget_hue, SIGNAL(hueChanged(int)), this, SLOT(onHueChanged(int)));
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::fit()
{
	ui->widget_image_view->scaleFit(0.98);
}

void MainWindow::on_horizontalSlider_size_valueChanged(int value)
{
	ui->widget->setBrushSize(value);
}

void MainWindow::on_horizontalSlider_softness_valueChanged(int value)
{
	ui->widget->setBrushSoftness(value / 100.0);
}

void MainWindow::onHueChanged(int hue)
{
	ui->widget_sb->setHue(hue);
}

void MainWindow::on_action_resize_triggered()
{
	QImage srcimage = ui->widget_image_view->image();
	QSize sz = srcimage.size();

	ResizeDialog dlg(this);
	dlg.setImageSize(sz);
	if (dlg.exec() == QDialog::Accepted) {
		sz = dlg.imageSize();
		unsigned int w = sz.width();
		unsigned int h = sz.height();
		if (w < 1) w = 1;
		if (h < 1) h = 1;
		QImage newimage = resizeImage(srcimage, w, h, EnlargeMethod::Bicubic);
		ui->widget_image_view->setImage(newimage);
		fit();
	}
}

void MainWindow::on_action_file_open_triggered()
{
	QByteArray ba;
	QString path = QFileDialog::getOpenFileName(this);
	QFile file(path);
	if (file.open(QFile::ReadOnly)) {
		ba = file.readAll();
	}
	ui->widget_image_view->setImage(QString(), ba);

	fit();
}

void MainWindow::on_action_file_save_as_triggered()
{
	QString path = QFileDialog::getSaveFileName(this);
	if (!path.isEmpty()) {
		QImage img = ui->widget_image_view->image();
		img.save(path);
	}
}

void MainWindow::on_action_filter_median_triggered()
{
	QImage image = ui->widget_image_view->image();
	image = filter_median(image, 10);
	ui->widget_image_view->setImage(image);
}

void MainWindow::on_action_filter_maximize_triggered()
{
	QImage image = ui->widget_image_view->image();
	image = filter_maximize(image, 10);
	ui->widget_image_view->setImage(image);
}

void MainWindow::on_action_filter_minimize_triggered()
{
	QImage image = ui->widget_image_view->image();
	image = filter_minimize(image, 10);
	ui->widget_image_view->setImage(image);
}

QImage filter_blur(QImage image, int radius);

void MainWindow::on_action_filter_blur_triggered()
{
	QImage image = ui->widget_image_view->image();
	int radius = 10;
	image = filter_blur(image, radius);
	image = filter_blur(image, radius);
	image = filter_blur(image, radius);
	ui->widget_image_view->setImage(image);
}


void MainWindow::on_action_filter_antialias_triggered()
{
	QImage image = ui->widget_image_view->image();
	filter_antialias(&image);
	ui->widget_image_view->setImage(image);
}

