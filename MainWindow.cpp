#include "MainWindow.h"
#include "ResizeDialog.h"
#include "antialias.h"
#include "median.h"
#include "resize.h"
#include "ui_MainWindow.h"
#include <QFileDialog>
#include <QPainter>
#include <stdint.h>

struct MainWindow::Private {
	Document doc;
	QImage selection;
};

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
	, m(new Private)
{
	ui->setupUi(this);
	ui->horizontalSlider_size->setValue(1);
	ui->horizontalSlider_softness->setValue(0);
	ui->widget->setBrushSize(1);
	ui->widget->setBrushSoftness(0 / 100.0);
	ui->widget_image_view->bind(this, ui->verticalScrollBar, ui->horizontalScrollBar);
	ui->widget_image_view->setMouseTracking(true);

	connect(ui->widget_hue, SIGNAL(hueChanged(int)), this, SLOT(onHueChanged(int)));

}

MainWindow::~MainWindow()
{
	delete m;
	delete ui;
}

Document *MainWindow::document()
{
	return &m->doc;
}

Document const *MainWindow::document() const
{
	return &m->doc;
}

void MainWindow::setImage(const QImage &image, bool fitview)
{
	document()->image = image.convertToFormat(QImage::Format_RGBA8888);
	ui->widget_image_view->update();
	if (fitview) {
		fitView();
	}
}

void MainWindow::setImage(QByteArray const &ba)
{
	QImage image;
	image.loadFromData(ba);
	setImage(image, true);
}

QImage MainWindow::renderImage(QRect const &r) const
{
	QImage img;
	if (!document()->image.isNull()) {
		int offset_x = m->doc.offset.x();
		int offset_y = m->doc.offset.y();
		QRect r2 = r.translated(-offset_x, -offset_y);
		img = document()->image.copy(r2);

#if 0
		if (!m->selection.isNull()) {
			QImage sel(r.width(), r.height(), QImage::Format_Grayscale8);
			sel.fill(Qt::black);
			int sx0 = r.x();
			int sy0 = r.y();
			int sx1 = r.x() + r.width();
			int sy1 = r.y() + r.height();
			int dx0 = 0;
			int dy0 = 0;
			int dx1 = sel.width();
			int dy1 = sel.height();
			if (sx0 < dx0) {
				int d = dx0 - sx0;
				dx0 += d;
			} else if (sx0 > dx0) {
				int d = sx0 - dx0;
				sx0 += d;
			}
			if (sy0 < dy0) {
				int d = dy0 - sy0;
				dy0 += d;
			} else if (sy0 > dy0) {
				int d = sy0 - dy0;
				sy0 += d;
			}
			if (sx1 < dx1) {
				int d = dx1 - sx1;
				dx1 -= d;
			} else if (sx1 > dx1) {
				int d = sx1 - dx1;
				sx1 -= d;
			}
			if (sy1 < dy1) {
				int d = dy1 - sy1;
				dy1 -= d;
			} else if (sy1 > dy1) {
				int d = sy1 - dy1;
				sy1 -= d;
			}
			{
				int w = dx1 - dx0;
				int h = dy1 - dy0;
				QPainter pr2(&sel);
				pr2.drawImage(QRect(dx0, dy0, w, h), m->selection, QRect(sx0, sy0, w, h));
			}
			{
				struct Pixel {
					uint8_t r;
					uint8_t g;
					uint8_t b;
					uint8_t a;
				};

				int w = r.width();
				int h = r.height();
				for (int y = 0; y < h; y++) {
					uint8_t const *s = sel.scanLine(y);
					Pixel *d = (Pixel *)img.scanLine(y);
					for (int x = 0; x < w; x++) {
						int a = s[x];
						int r = d[x].r;
						int r2 = r / 4 + 192;
						d[x].r = (r * a + r2 * (255 - a)) / 255;
					}
				}
			}
		}
#endif
	}
	return img;
}

QRect MainWindow::selectionRect() const
{
	return m->selection.rect();
}

void MainWindow::fitView()
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
	QImage srcimage = document()->image;
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
		setImage(newimage, true);
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
	setImage(ba);

	fitView();
}

void MainWindow::on_action_file_save_as_triggered()
{
	QString path = QFileDialog::getSaveFileName(this);
	if (!path.isEmpty()) {
		QImage img = document()->image;
		img.save(path);
	}
}

void MainWindow::on_action_filter_median_triggered()
{
	QImage image = document()->image;
	image = filter_median(image, 10);
	setImage(image, false);
}

void MainWindow::on_action_filter_maximize_triggered()
{
	QImage image = document()->image;
	image = filter_maximize(image, 10);
	setImage(image, false);
}

void MainWindow::on_action_filter_minimize_triggered()
{
	QImage image = document()->image;
	image = filter_minimize(image, 10);
	setImage(image, false);
}

QImage filter_blur(QImage image, int radius);

void MainWindow::on_action_filter_blur_triggered()
{
	QImage image = document()->image;
	int radius = 10;
	image = filter_blur(image, radius);
	image = filter_blur(image, radius);
	image = filter_blur(image, radius);
	setImage(image, false);
}


void MainWindow::on_action_filter_antialias_triggered()
{
	QImage image = document()->image;
	filter_antialias(&image);
	setImage(image, false);
}


void MainWindow::on_horizontalScrollBar_valueChanged(int value)
{
	(void)value;
	ui->widget_image_view->refrectScrollBar();
}

void MainWindow::on_verticalScrollBar_valueChanged(int value)
{
	(void)value;
	ui->widget_image_view->refrectScrollBar();
}

void MainWindow::on_action_trim_triggered()
{
	QRect r_doc = m->doc.image.rect();
	QRect r_sel = selectionRect();
	int sx0 = r_doc.x();
	int sy0 = r_doc.y();
	int sx1 = r_doc.x() + r_doc.width();
	int sy1 = r_doc.y() + r_doc.height();
	int dx0 = r_sel.x();
	int dy0 = r_sel.y();
	int dx1 = r_sel.width();
	int dy1 = r_sel.height();
	if (sx0 < dx0) {
		int d = dx0 - sx0;
		dx0 += d;
	} else if (sx0 > dx0) {
		int d = sx0 - dx0;
		sx0 += d;
	}
	if (sy0 < dy0) {
		int d = dy0 - sy0;
		dy0 += d;
	} else if (sy0 > dy0) {
		int d = sy0 - dy0;
		sy0 += d;
	}
	if (sx1 < dx1) {
		int d = dx1 - sx1;
		dx1 -= d;
	} else if (sx1 > dx1) {
		int d = sx1 - dx1;
		sx1 -= d;
	}
	if (sy1 < dy1) {
		int d = dy1 - sy1;
		dy1 -= d;
	} else if (sy1 > dy1) {
		int d = sy1 - dy1;
		sy1 -= d;
	}
	QRect r(dx0, dy0, dx1 - dx0, dy1 - dy0);
	QImage img = m->doc.image.copy(r);
	{
		int w = img.width();
		int h = img.height();
		m->selection = QImage(w, h, QImage::Format_Grayscale8);
	}
	setImage(img, true);
}

