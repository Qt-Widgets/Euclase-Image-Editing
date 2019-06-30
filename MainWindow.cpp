#include "AlphaBlend.h"
#include "Document.h"
#include "MainWindow.h"
#include "ResizeDialog.h"
#include "RoundBrushGenerator.h"
#include "antialias.h"
#include "median.h"
#include "resize.h"
#include "ui_MainWindow.h"
#include <QFileDialog>
#include <QPainter>
#include <stdint.h>
#include <QKeyEvent>
#include <QDebug>

struct MainWindow::Private {
	Document doc;
	QColor foreground_color;
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

	setForegroundColor(Qt::red);
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

int MainWindow::documentWidth() const
{
	auto const *d = document();
	return d ? d->width() : 0;
}

int MainWindow::documentHeight() const
{
	auto const *d = document();
	return d ? d->height() : 0;
}

void MainWindow::setForegroundColor(const QColor &color)
{
	m->foreground_color = color;
}

QColor MainWindow::foregroundColor() const
{
	return m->foreground_color;
}

void MainWindow::setImage(const QImage &image, bool fitview)
{
	document()->current_layer()->image = image.convertToFormat(QImage::Format_RGBA8888);
	if (1) {
		int w = documentWidth();
		int h = documentHeight();
		document()->selection_layer()->image = QImage(w, h, QImage::Format_Grayscale8);
		document()->selection_layer()->image.fill(Qt::black);
		QPainter pr(&document()->selection_layer()->image);
		pr.setRenderHint(QPainter::Antialiasing);
		pr.setPen(Qt::NoPen);
		pr.setBrush(Qt::white);
		pr.drawEllipse(0, 0, w - 1, h - 1);
	}
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
	return document()->render(r);
}

QRect MainWindow::selectionRect() const
{
	return document()->selection_layer()->image.rect();
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
	ui->widget_color->setHue(hue);
}

void MainWindow::on_action_resize_triggered()
{
	QImage srcimage = document()->current_layer()->image;
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

void MainWindow::openFile(QString const &path)
{
	QByteArray ba;

	QFile file(path);
	if (file.open(QFile::ReadOnly)) {
		ba = file.readAll();
	}
	setImage(ba);

	fitView();
}

void MainWindow::on_action_file_open_triggered()
{
	QString path = QFileDialog::getOpenFileName(this);
	openFile(path);
}

void MainWindow::on_action_file_save_as_triggered()
{
	QString path = QFileDialog::getSaveFileName(this);
	if (!path.isEmpty()) {
		QImage img = document()->current_layer()->image;
		img.save(path);
	}
}

void MainWindow::on_action_filter_median_triggered()
{
	QImage image = document()->current_layer()->image;
	image = filter_median(image, 10);
	setImage(image, false);
}

void MainWindow::on_action_filter_maximize_triggered()
{
	QImage image = document()->current_layer()->image;
	image = filter_maximize(image, 10);
	setImage(image, false);
}

void MainWindow::on_action_filter_minimize_triggered()
{
	QImage image = document()->current_layer()->image;
	image = filter_minimize(image, 10);
	setImage(image, false);
}

QImage filter_blur(QImage image, int radius);

void MainWindow::on_action_filter_blur_triggered()
{
	QImage image = document()->current_layer()->image;
	int radius = 10;
	image = filter_blur(image, radius);
	image = filter_blur(image, radius);
	image = filter_blur(image, radius);
	setImage(image, false);
}


void MainWindow::on_action_filter_antialias_triggered()
{
	QImage image = document()->current_layer()->image;
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
	QRect r_doc = document()->current_layer()->image.rect();
	QRect r_sel = selectionRect();
	int sx0 = r_doc.x();
	int sy0 = r_doc.y();
	int sx1 = r_doc.x() + r_doc.width();
	int sy1 = r_doc.y() + r_doc.height();
	int dx0 = r_sel.x();
	int dy0 = r_sel.y();
	int dx1 = r_sel.width();
	int dy1 = r_sel.height();
	if (sx0 < dx0) { dx0 += dx0 - sx0; } else if (sx0 > dx0) { sx0 += sx0 - dx0; }
	if (sy0 < dy0) { dy0 += dy0 - sy0; } else if (sy0 > dy0) { sy0 += sy0 - dy0; }
	if (sx1 < dx1) { dx1 -= dx1 - sx1; } else if (sx1 > dx1) { sx1 -= sx1 - dx1; }
	if (sy1 < dy1) { dy1 -= dy1 - sy1; } else if (sy1 > dy1) { sy1 -= sy1 - dy1; }
	QRect r(dx0, dy0, dx1 - dx0, dy1 - dy0);
	QImage img = document()->current_layer()->image.copy(r);
	{
		int w = img.width();
		int h = img.height();
		document()->selection_layer()->image = QImage(w, h, QImage::Format_Grayscale8);
	}
	setImage(img, true);
}

void MainWindow::applyBrush(Document::Layer const &layer, bool update)
{
	document()->paint(layer, foregroundColor());

	if (update) {
		ui->widget_image_view->update();
	}
}

void MainWindow::drawBrush(double x, double y)
{
	int size = 85;
	double softness = 1.0;
	RoundBrushGenerator brush(size, softness);
	int x0 = floor(x - size / 2.0);
	int y0 = floor(y - size / 2.0);
	int x1 = ceil(x + size / 2.0);
	int y1 = ceil(y + size / 2.0);
	int w = x1 - x0;
	int h = y1 - y0;
	QImage image(w, h, QImage::Format_Grayscale8);
	image.fill(Qt::black);
	for (int i = 0; i < h; i++) {
		uint8_t *dst = reinterpret_cast<uint8_t *>(image.scanLine(i));
		for (int j = 0; j < w; j++) {
			double tx = x0 + j - x + 0.5;
			double ty = y0 + i - y + 0.5;
			double value = brush.level(tx, ty);
			int v = (int)(value  * 255);
			dst[j] = v;
		}
	}
	Document::Layer layer;
	layer.image = image;
	layer.offset = QPoint(x0, y0);
	applyBrush(layer, true);
}

void MainWindow::onPenDown(double x, double y)
{
	drawBrush(x, y);
}

void MainWindow::onPenStroke(double x, double y)
{
	drawBrush(x, y);
}

void MainWindow::onPenUp(double x, double y)
{
}


void MainWindow::onMouseLeftButtonPress(int x, int y)
{
	QPointF pos(x + 0.5, y + 0.5);
	pos = ui->widget_image_view->mapFromViewport(pos);
	onPenDown(pos.x(), pos.y());
}

void MainWindow::onMouseMove(int x, int y, bool leftbutton)
{
	if (leftbutton) {
		QPointF pos(x + 0.5, y + 0.5);
		pos = ui->widget_image_view->mapFromViewport(pos);
		onPenStroke(pos.x(), pos.y());
	}
}

void MainWindow::onMouseLeftButtonRelase(int x, int y, bool leftbutton)
{
	if (leftbutton) {
		QPointF pos(x + 0.5, y + 0.5);
		pos = ui->widget_image_view->mapFromViewport(pos);
		onPenUp(pos.x(), pos.y());
	}
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
	int k = event->key();
	if (k == Qt::Key_T) {
		if (event->modifiers() & Qt::ControlModifier) {
			drawBrush();
		}
	}

}

void MainWindow::drawBrush()
{
	openFile("../lena_std.png");
}
