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
#include <QElapsedTimer>

struct MainWindow::Private {
	Document doc;
	QColor foreground_color;
	Brush current_brush;

	double brush_next_distance = 0;
	double brush_span = 4;
	double brush_t = 0;
	QPointF brush_bezier[4];
};

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
	, m(new Private)
{
	ui->setupUi(this);
	ui->horizontalSlider_size->setValue(1);
	ui->horizontalSlider_softness->setValue(0);
	ui->widget_image_view->bind(this, ui->verticalScrollBar, ui->horizontalScrollBar);
	ui->widget_image_view->setMouseTracking(true);

	ui->horizontalSlider_rgb_r->setColorType(ColorSlider::RGB_R);
	ui->horizontalSlider_rgb_g->setColorType(ColorSlider::RGB_G);
	ui->horizontalSlider_rgb_b->setColorType(ColorSlider::RGB_B);
	ui->horizontalSlider_hsv_h->setColorType(ColorSlider::HSV_H);
	ui->horizontalSlider_hsv_s->setColorType(ColorSlider::HSV_S);
	ui->horizontalSlider_hsv_v->setColorType(ColorSlider::HSV_V);

	ui->tabWidget->setCurrentWidget(ui->tab_color_rgb);

	connect(ui->widget_hue, SIGNAL(hueChanged(int)), this, SLOT(onHueChanged(int)));

	setForegroundColor(Qt::red);

	{
		Brush b;
		b.size = 85;
		b.softness = 1.0;
		setCurrentBrush(b);
	}
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

	auto Set = [&](int v, ColorSlider *slider, QSpinBox *spin){
		bool f1 = slider->blockSignals(true);
		slider->setColor(m->foreground_color);
		slider->setValue(v);
		slider->blockSignals(f1);
		bool f2 = spin->blockSignals(true);
		spin->setValue(v);
		spin->blockSignals(f2);
	};
	Set(color.red(), ui->horizontalSlider_rgb_r, ui->spinBox_rgb_r);
	Set(color.green(), ui->horizontalSlider_rgb_g, ui->spinBox_rgb_g);
	Set(color.blue(), ui->horizontalSlider_rgb_b, ui->spinBox_rgb_b);
	Set(color.hue(), ui->horizontalSlider_hsv_h, ui->spinBox_hsv_h);
	Set(color.saturation(), ui->horizontalSlider_hsv_s, ui->spinBox_hsv_s);
	Set(color.value(), ui->horizontalSlider_hsv_v, ui->spinBox_hsv_v);
}

QColor MainWindow::foregroundColor() const
{
	return m->foreground_color;
}

void MainWindow::setCurrentBrush(const Brush &brush)
{
	m->current_brush = brush;
	m->brush_span = std::max(brush.size / 8.0, 0.5);

	bool f1 = ui->horizontalSlider_size->blockSignals(true);
	bool f2 = ui->horizontalSlider_softness->blockSignals(true);

	ui->widget_brush->setBrush_(brush);

	ui->horizontalSlider_size->setValue(brush.size);
	ui->horizontalSlider_softness->setValue(brush.softness * 100);

	ui->horizontalSlider_softness->blockSignals(f2);
	ui->horizontalSlider_size->blockSignals(f1);
}

Brush const &MainWindow::currentBrush() const
{
	return m->current_brush;
}

void MainWindow::setImage(const QImage &image, bool fitview)
{
	int w = image.width();
	int h = image.height();

//	document()->current_layer()->image() = QImage(w, h, QImage::Format_RGBA8888);
//	document()->current_layer()->image().fill(Qt::transparent);
	document()->current_layer()->create(w, h);

	Document::Layer layer(w, h);
	layer.image() = image;
	document()->render(document()->current_layer(), layer, nullptr, QColor());

	if (1) {
		int w = documentWidth();
		int h = documentHeight();
		document()->selection_layer()->image() = QImage(w, h, QImage::Format_Grayscale8);
		document()->selection_layer()->image().fill(Qt::black);
		QPainter pr(&document()->selection_layer()->image());
		pr.setRenderHint(QPainter::Antialiasing);
		pr.setPen(Qt::NoPen);
		pr.setBrush(Qt::white);
//		pr.drawEllipse(0, 0, 3, 3);
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
	return document()->selection_layer()->image().rect();
}

void MainWindow::fitView()
{
	ui->widget_image_view->scaleFit(0.98);
}

void MainWindow::on_horizontalSlider_size_valueChanged(int value)
{
	ui->widget_brush->setBrushSize(value);
}

void MainWindow::on_horizontalSlider_softness_valueChanged(int value)
{
	ui->widget_brush->setBrushSoftness(value / 100.0);
}

void MainWindow::onHueChanged(int hue)
{
	ui->widget_color->setHue(hue);
}

void MainWindow::on_action_resize_triggered()
{
	QImage srcimage = document()->current_layer()->image();
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
		QImage img = document()->current_layer()->image();
		img.save(path);
	}
}

void MainWindow::on_action_filter_median_triggered()
{
	QImage image = document()->current_layer()->image();
	image = filter_median(image, 10);
	setImage(image, false);
}

void MainWindow::on_action_filter_maximize_triggered()
{
	QImage image = document()->current_layer()->image();
	image = filter_maximize(image, 10);
	setImage(image, false);
}

void MainWindow::on_action_filter_minimize_triggered()
{
	QImage image = document()->current_layer()->image();
	image = filter_minimize(image, 10);
	setImage(image, false);
}

QImage filter_blur(QImage image, int radius);

void MainWindow::on_action_filter_blur_triggered()
{
	QImage image = document()->current_layer()->image();
	int radius = 10;
	image = filter_blur(image, radius);
	image = filter_blur(image, radius);
	image = filter_blur(image, radius);
	setImage(image, false);
}


void MainWindow::on_action_filter_antialias_triggered()
{
	QImage image = document()->current_layer()->image();
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
	QRect r_doc = document()->current_layer()->image().rect();
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
	QImage img = document()->current_layer()->image().copy(r);
	{
		int w = img.width();
		int h = img.height();
		document()->selection_layer()->image() = QImage(w, h, QImage::Format_Grayscale8);
	}
	setImage(img, true);
}

void MainWindow::updateImageView()
{
	ui->widget_image_view->paintViewLater();
}

void MainWindow::paintColor(Document::Layer const &layer)
{
	document()->paint(layer, foregroundColor());
}

void MainWindow::drawBrush(bool one)
{
	auto Put = [&](QPointF const &pt, Brush const &brush){
		RoundBrushGenerator shape(brush.size, brush.softness);
		int x0 = floor(pt.x() - brush.size / 2.0);
		int y0 = floor(pt.y() - brush.size / 2.0);
		int x1 = ceil(pt.x() + brush.size / 2.0);
		int y1 = ceil(pt.y() + brush.size / 2.0);
		int w = x1 - x0;
		int h = y1 - y0;
		QImage image(w, h, QImage::Format_Grayscale8);
		image.fill(Qt::black);
		for (int i = 0; i < h; i++) {
			uint8_t *dst = reinterpret_cast<uint8_t *>(image.scanLine(i));
			for (int j = 0; j < w; j++) {
				double tx = x0 + j - pt.x() + 0.5;
				double ty = y0 + i - pt.y() + 0.5;
				double value = shape.level(tx, ty);
				int v = (int)(value  * 255);
				dst[j] = v;
			}
		}
		Document::Layer layer(image.width(), image.height());
		layer.image() = image;
		layer.offset() = QPoint(x0, y0);
		paintColor(layer);
	};

	auto Point = [&](double t){
		return euclase::cubicBezierPoint(m->brush_bezier[0], m->brush_bezier[1], m->brush_bezier[2], m->brush_bezier[3], t);
	};

	QPointF pt0 = Point(m->brush_t);
	if (one) {
		Put(pt0, currentBrush());
		m->brush_next_distance = m->brush_span;
	} else {
		do {
			if (m->brush_next_distance == 0) {
				Put(pt0, currentBrush());
				m->brush_next_distance = m->brush_span;
			}
			double t = std::min(m->brush_t + (1.0 / 16), 1.0);
			QPointF pt1 = Point(t);
			double dx = pt0.x() - pt1.x();
			double dy = pt0.y() - pt1.y();
			double d = hypot(dx, dy);
			if (m->brush_next_distance > d) {
				m->brush_next_distance -= d;
				m->brush_t = t;
				pt0 = pt1;
			} else {
				m->brush_t += (t - m->brush_t) * m->brush_next_distance / d;
				m->brush_t = std::min(m->brush_t, 1.0);
				m->brush_next_distance = 0;
				pt0 = Point(m->brush_t);
			}
		} while (m->brush_t < 1.0);
	}

	m->brush_t = 0;
	updateImageView();
}

void MainWindow::onPenDown(double x, double y)
{
	m->brush_bezier[0] = m->brush_bezier[1] = m->brush_bezier[2] = m->brush_bezier[3] = QPointF(x, y);
	m->brush_next_distance = 0;
	m->brush_t = 0;
	drawBrush(true);
}

void MainWindow::onPenStroke(double x, double y)
{
	m->brush_bezier[0] = m->brush_bezier[3];
	m->brush_bezier[3] = QPointF(x, y);
	x = (m->brush_bezier[0].x() * 2 + m->brush_bezier[3].x()) / 3;
	y = (m->brush_bezier[0].y() * 2 + m->brush_bezier[3].y()) / 3;
	m->brush_bezier[1] = QPointF(x, y);
	x = (m->brush_bezier[0].x() + m->brush_bezier[3].x() * 2) / 3;
	y = (m->brush_bezier[0].y() + m->brush_bezier[3].y() * 2) / 3;
	m->brush_bezier[2] = QPointF(x, y);

	drawBrush(false);
}

void MainWindow::onPenUp(double x, double y)
{
	(void)x;
	(void)y;
	updateImageView();
	m->brush_next_distance = 0;
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
//		updateImageView();
	}
}

void MainWindow::setColorRed(int value)
{
	QColor c = foregroundColor();
	c = QColor(value, c.green(), c.blue());
	setForegroundColor(c);
}

void MainWindow::setColorGreen(int value)
{
	QColor c = foregroundColor();
	c = QColor(c.red(), value, c.blue());
	setForegroundColor(c);
}

void MainWindow::setColorBlue(int value)
{
	QColor c = foregroundColor();
	c = QColor(c.red(), c.green(), value);
	setForegroundColor(c);
}

void MainWindow::setColorHue(int value)
{
	QColor c = foregroundColor();
	c = QColor::fromHsv(value, c.saturation(), c.value());
	setForegroundColor(c);
}

void MainWindow::setColorSaturation(int value)
{
	QColor c = foregroundColor();
	c = QColor::fromHsv(c.hue(), value, c.value());
	setForegroundColor(c);
}

void MainWindow::setColorValue(int value)
{
	QColor c = foregroundColor();
	c = QColor::fromHsv(c.hue(), c.saturation(), value);
	setForegroundColor(c);
}

void MainWindow::on_horizontalSlider_rgb_r_valueChanged(int value)
{
	setColorRed(value);
}

void MainWindow::on_horizontalSlider_rgb_g_valueChanged(int value)
{
	setColorGreen(value);
}

void MainWindow::on_horizontalSlider_rgb_b_valueChanged(int value)
{
	setColorBlue(value);
}

void MainWindow::on_spinBox_rgb_r_valueChanged(int value)
{
	setColorRed(value);
}

void MainWindow::on_spinBox_rgb_g_valueChanged(int value)
{
	setColorGreen(value);
}

void MainWindow::on_spinBox_rgb_b_valueChanged(int value)
{
	setColorBlue(value);
}

void MainWindow::on_horizontalSlider_hsv_h_valueChanged(int value)
{
	setColorHue(value);
}

void MainWindow::on_horizontalSlider_hsv_s_valueChanged(int value)
{
	setColorSaturation(value);
}

void MainWindow::on_horizontalSlider_hsv_v_valueChanged(int value)
{
	setColorValue(value);
}

void MainWindow::on_spinBox_hsv_h_valueChanged(int value)
{
	setColorHue(value);
}

void MainWindow::on_spinBox_hsv_s_valueChanged(int value)
{
	setColorSaturation(value);
}

void MainWindow::on_spinBox_hsv_v_valueChanged(int value)
{
	setColorValue(value);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
	int k = event->key();
	if (k == Qt::Key_T) {
		if (event->modifiers() & Qt::ControlModifier) {
			test();
		}
	}

}

void MainWindow::test()
{
	QImage image(100, 100, QImage::Format_Grayscale8);
	image.fill(Qt::black);
	{
		QPainter pr(&image);
		pr.setRenderHint(QPainter::Antialiasing);
		pr.setPen(Qt::NoPen);
		pr.setBrush(Qt::white);
		pr.drawEllipse(0, 0, 99, 99);
	}

	Document::Layer layer(image.width(), image.height());
	layer.image() = image;
	layer.offset() = QPoint(14, 14);
	paintColor(layer);

	updateImageView();
}


