#include "AlphaBlend.h"
#include "Document.h"
#include "MainWindow.h"
#include "NewDialog.h"
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
#include <QBitmap>
#include <QScreen>
#include <QClipboard>
#include <QElapsedTimer>

struct MainWindow::Private {
	Document doc;
	QColor foreground_color;
	Brush current_brush;

	double brush_next_distance = 0;
	double brush_span = 4;
	double brush_t = 0;
	QPointF brush_bezier[4];

	MainWindow::Tool current_tool;

	QPoint start_vpt;
	QPointF anchor_dpt;
	QPointF topleft_dpt;
	QPointF bottomright_dpt;
	QPointF rect_topleft_dpt;
	QPointF rect_bottomright_dpt;


	MainWindow::RectHandle rect_handle = MainWindow::RectHandle::None;
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

	ui->toolButton_scroll->setCheckable(true);
	ui->toolButton_brush->setCheckable(true);
	ui->toolButton_rect->setCheckable(true);
	ui->toolButton_scroll->click();

	ui->horizontalSlider_rgb_r->setVisualType(ColorSlider::RGB_R);
	ui->horizontalSlider_rgb_g->setVisualType(ColorSlider::RGB_G);
	ui->horizontalSlider_rgb_b->setVisualType(ColorSlider::RGB_B);
	ui->horizontalSlider_hsv_h->setVisualType(ColorSlider::HSV_H);
	ui->horizontalSlider_hsv_s->setVisualType(ColorSlider::HSV_S);
	ui->horizontalSlider_hsv_v->setVisualType(ColorSlider::HSV_V);

	ui->horizontalSlider_size->setVisualType(BrushSlider::SIZE);
	ui->horizontalSlider_softness->setVisualType(BrushSlider::SOFTNESS);

	ui->tabWidget->setCurrentWidget(ui->tab_color_hsv);

//	connect(ui->widget_hue, &HueWidget::hueChanged, this, &MainWindow::onHueChanged);
	connect(ui->widget_color, &SaturationBrightnessWidget::changeColor, this, &MainWindow::setForegroundColor);

	connect(ui->widget_image_view, &ImageViewWidget::scaleChanged, [&](double scale){
		ui->widget_brush->changeScale(scale);
	});

	setForegroundColor(Qt::red);

	{
		Brush b;
		b.size = 85;
		b.softness = 1.0;
		setCurrentBrush(b);
	}

	ui->widget_image_view->setFocus();
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

QMutex *MainWindow::synchronizer() const
{
	return ui->widget_image_view->synchronizer();
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

//	{
//		bool f = ui->widget_hue->blockSignals(true);
//		ui->widget_hue->setHue(color.hue());
//		ui->widget_hue->blockSignals(f);
//	}
	{
		bool f = ui->widget_color->blockSignals(true);
		ui->widget_color->setHue(color.hue());
		ui->widget_color->blockSignals(f);
	}
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
	bool f3 = ui->spinBox_brush_size->blockSignals(true);
	bool f4 = ui->horizontalSlider_softness->blockSignals(true);

	ui->widget_brush->setBrush_(brush);

	ui->horizontalSlider_size->setValue(brush.size);
	ui->spinBox_brush_size->setValue(brush.size);
	ui->horizontalSlider_softness->setValue(brush.softness * 100);
	ui->spinBox_brush_softness->setValue(brush.softness * 100);

	ui->horizontalSlider_softness->blockSignals(f4);
	ui->spinBox_brush_size->blockSignals(f3);
	ui->horizontalSlider_softness->blockSignals(f2);
	ui->horizontalSlider_size->blockSignals(f1);
}

Brush const &MainWindow::currentBrush() const
{
	return m->current_brush;
}

void MainWindow::setImage(const QImage &image, bool fitview)
{
	clearSelection();
	int w = image.width();
	int h = image.height();
	document()->setSize(QSize(w, h));
	document()->current_layer()->clear(nullptr);
	document()->current_layer()->tile_mode_ = true;

	Document::Layer layer;
	layer.setImage(QPoint(0, 0), image);
	Document::RenderOption opt;
	opt.mode = Document::RenderOption::DirectCopy;
	document()->renderToLayer(document()->current_layer(), layer, nullptr, opt, ui->widget_image_view->synchronizer(), nullptr);

	ui->widget_image_view->update();

	if (fitview) {
		fitView();
	} else {
		updateImageView();
	}
	onSelectionChanged();
}

void MainWindow::setImage(QByteArray const &ba, bool fitview)
{
	QImage image;
	image.loadFromData(ba);
	setImage(image, fitview);
}

QImage MainWindow::renderImage(QRect const &r, bool quickmask, bool *abort) const
{
	return document()->renderToLayer(r, quickmask, ui->widget_image_view->synchronizer(), abort);
}

SelectionOutlineBitmap MainWindow::renderSelectionOutline(bool *abort) const
{
	return ui->widget_image_view->renderSelectionOutlineBitmap(abort);
}

QRect MainWindow::selectionRect() const
{
	return document()->selection_layer()->rect();
}

void MainWindow::fitView()
{
	ui->widget_image_view->scaleFit(0.98);
}

void MainWindow::on_horizontalSlider_size_valueChanged(int value)
{
	ui->widget_brush->setBrushSize(value);
}

void MainWindow::on_spinBox_brush_size_valueChanged(int value)
{
	ui->widget_brush->setBrushSize(value);
}

void MainWindow::on_horizontalSlider_softness_valueChanged(int value)
{
	ui->widget_brush->setBrushSoftness(value / 100.0);
}

void MainWindow::on_spinBox_brush_softness_valueChanged(int value)
{
	ui->widget_brush->setBrushSoftness(value / 100.0);
}

void MainWindow::onHueChanged(int hue)
{
	ui->widget_color->setHue(hue);
}

void MainWindow::on_action_resize_triggered()
{
	QImage srcimage = renderFilterTargetImage();
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
	setImage(ba, true);
}

void MainWindow::on_action_file_open_triggered()
{
	QString path = QFileDialog::getOpenFileName(this);
	if (path.isEmpty()) return;
	openFile(path);
}

void MainWindow::on_action_file_save_as_triggered()
{
	QString path = QFileDialog::getSaveFileName(this);
	if (!path.isEmpty()) {
		QSize sz = document()->size();
		QImage img = document()->renderToLayer(QRect(0, 0, sz.width(), sz.height()), false, synchronizer(), nullptr);
		img.save(path);
	}
}

QImage MainWindow::renderFilterTargetImage()
{
	QSize sz = document()->size();
	QImage image = renderImage(QRect(0, 0, sz.width(), sz.height()), false, nullptr);
	return image;
}

void MainWindow::on_action_filter_median_triggered()
{
	QImage image = renderFilterTargetImage();
	image = filter_median(image, 10);
	setImage(image, false);
}

void MainWindow::on_action_filter_maximize_triggered()
{
	QImage image = renderFilterTargetImage();
	image = filter_maximize(image, 10);
	setImage(image, false);
}

void MainWindow::on_action_filter_minimize_triggered()
{
	QImage image = renderFilterTargetImage();
	image = filter_minimize(image, 10);
	setImage(image, false);
}

QImage filter_blur(QImage image, int radius);

void MainWindow::on_action_filter_blur_triggered()
{
	QImage image = renderFilterTargetImage();
	int radius = 10;
	image = filter_blur(image, radius);
	image = filter_blur(image, radius);
	image = filter_blur(image, radius);
	setImage(image, false);
}


void MainWindow::on_action_filter_antialias_triggered()
{
	QImage image = renderFilterTargetImage();
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

QImage MainWindow::selectedImage() const
{
	QRect r = selectionRect();
	return document()->crop(r, synchronizer(), nullptr);
}

void MainWindow::on_action_trim_triggered()
{
	QImage image = selectedImage();
	setImage(image, true);
}

void MainWindow::updateImageView()
{
	ui->widget_image_view->paintViewLater(true, false);
}

void MainWindow::updateSelectionOutline()
{
	ui->widget_image_view->paintViewLater(false, true);
}

void MainWindow::onSelectionChanged()
{
	updateSelectionOutline();
}

void MainWindow::clearSelection()
{
	document()->clearSelection(synchronizer());
}

void MainWindow::paintLayer(Operation op, Document::Layer const &layer)
{
	if (op == Operation::PaintToCurrentLayer) {
		Document::RenderOption opt;
		opt.brush_color = foregroundColor();
		document()->paintToCurrentLayer(layer, opt, ui->widget_image_view->synchronizer(), nullptr);
		return;
	}
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
		Document::Layer layer;
		layer.setImage(QPoint(x0, y0), image);
		paintLayer(Operation::PaintToCurrentLayer, layer);
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

QPointF MainWindow::pointOnDocument(int x, int y) const
{
	QPointF pos(x + 0.5, y + 0.5);
	return ui->widget_image_view->mapFromViewportToDocument(pos);
}

QPointF MainWindow::mapFromViewportToDocument(QPointF const &pt) const
{
	return ui->widget_image_view->mapFromViewportToDocument(pt);
}

QPointF MainWindow::mapFromDocumentToViewport(QPointF const &pt) const
{
	return ui->widget_image_view->mapFromDocumentToViewport(pt);
}

MainWindow::RectHandle MainWindow::rectHitTest(QPoint const &pt) const
{
	const int D = 100;

	QPointF topleft = mapFromDocumentToViewport(m->topleft_dpt);
	QPointF bottomright = mapFromDocumentToViewport(m->bottomright_dpt);
	if (topleft.x() > bottomright.x()) std::swap(topleft.rx(), bottomright.rx());
	if (topleft.y() > bottomright.y()) std::swap(topleft.ry(), bottomright.ry());
	int x0 = topleft.x();
	int y0 = topleft.y();
	int x1 = bottomright.x() + 1;
	int y1 = bottomright.y() + 1;

	int d, dx, dy;

	dx = pt.x() - x1;
	dy = pt.y() - y1;
	d = dx * dx + dy * dy;
	if (d < D) {
		return RectHandle::BottomRight;
	}

	dx = pt.x() - x0;
	dy = pt.y() - y1;
	d = dx * dx + dy * dy;
	if (d < D) {
		return RectHandle::BottomLeft;
	}

	dx = pt.x() - x1;
	dy = pt.y() - y0;
	d = dx * dx + dy * dy;
	if (d < D) {
		return RectHandle::TopRight;
	}

	dx = pt.x() - x0;
	dy = pt.y() - y0;
	d = dx * dx + dy * dy;
	if (d < D) {
		return RectHandle::TopLeft;
	}

	dx = pt.x() - (x0 + x1) / 2;
	dy = pt.y() - y0;
	d = dx * dx + dy * dy;
	if (d < D) {
		return RectHandle::Top;
	}

	dx = pt.x() - x0;
	dy = pt.y() - (y0 + y1) / 2;
	d = dx * dx + dy * dy;
	if (d < D) {
		return RectHandle::Left;
	}

	dx = pt.x() - x1;
	dy = pt.y() - (y0 + y1) / 2;
	d = dx * dx + dy * dy;
	if (d < D) {
		return RectHandle::Right;
	}

	dx = pt.x() - (x0 + x1) / 2;
	dy = pt.y() - y1;
	d = dx * dx + dy * dy;
	if (d < D) {
		return RectHandle::Bottom;
	}

	return RectHandle::None;
}

void MainWindow::setRect()
{
	double x0 = floor(m->topleft_dpt.x());
	double y0 = floor(m->topleft_dpt.y());
	double x1 = floor(m->bottomright_dpt.x());
	double y1 = floor(m->bottomright_dpt.y());
	if (x0 > x1) std::swap(x0, x1);
	if (y0 > y1) std::swap(y0, y1);
	m->rect_topleft_dpt = { x0, y0 };
	m->rect_bottomright_dpt = { x1 + 1, y1 + 1 };
	ui->widget_image_view->showRect(m->rect_topleft_dpt, m->rect_bottomright_dpt);
}

bool MainWindow::onMouseLeftButtonPress(int x, int y)
{
	m->start_vpt = QPoint(x, y);

	Tool tool = currentTool();
	if (tool == Tool::Scroll) return false;

	if (tool == Tool::Brush) {
		QPointF pos = pointOnDocument(x, y);
		onPenDown(pos.x(), pos.y());
		return true;
	}

	if (tool == Tool::Rect) {
		m->rect_handle = rectHitTest(m->start_vpt);
		if (m->rect_handle != RectHandle::None) {
			m->topleft_dpt += QPointF(0.01, 0.01);
			m->bottomright_dpt += QPointF(-0.01, -0.01);
			double x0 = m->topleft_dpt.x();
			double y0 = m->topleft_dpt.y();
			double x1 = m->bottomright_dpt.x();
			double y1 = m->bottomright_dpt.y();
			if (m->rect_handle == RectHandle::TopLeft) {
				m->anchor_dpt = m->topleft_dpt;
			} else if (m->rect_handle == RectHandle::TopRight) {
				m->anchor_dpt = QPointF(x1, y0);
			} else if (m->rect_handle == RectHandle::BottomLeft) {
				m->anchor_dpt = QPointF(x0, y1);
			} else if (m->rect_handle == RectHandle::BottomRight) {
				m->anchor_dpt = m->bottomright_dpt;
			} else if (m->rect_handle == RectHandle::Top) {
				m->anchor_dpt = QPointF((x0 + x1) / 2, y0);
			} else if (m->rect_handle == RectHandle::Left) {
				m->anchor_dpt = QPointF(x0, (y0 + y1) / 2);
			} else if (m->rect_handle == RectHandle::Right) {
				m->anchor_dpt = QPointF(x1, (y0 + y1) / 2);
			} else if (m->rect_handle == RectHandle::Bottom) {
				m->anchor_dpt = QPointF((x0 + x1) / 2, y1);
			}
		}
		if (m->rect_handle == RectHandle::None) {
			m->rect_handle = RectHandle::BottomRight;
			m->anchor_dpt = mapFromViewportToDocument(m->start_vpt);
			m->topleft_dpt = m->bottomright_dpt = m->anchor_dpt;
			setRect();
		}
		return true;
	}

	return false;
}

bool MainWindow::onMouseMove(int x, int y, bool leftbutton)
{
	Tool tool = currentTool();
	if (tool == Tool::Scroll) return false;

	if (tool == Tool::Brush) {
		if (leftbutton) {
			QPointF pos = pointOnDocument(x, y);
			onPenStroke(pos.x(), pos.y());
			return true;
		}
	}

	if (tool == Tool::Rect) {
		if (leftbutton) {
			if (m->rect_handle != RectHandle::None) {
				QPointF pt = mapFromViewportToDocument(mapFromDocumentToViewport(m->anchor_dpt) + QPointF(x, y) - m->start_vpt);
				if (m->rect_handle == RectHandle::TopLeft) {
					m->topleft_dpt = pt;
				} else if (m->rect_handle == RectHandle::BottomRight) {
					m->bottomright_dpt = pt;
				} else if (m->rect_handle == RectHandle::TopRight) {
					m->topleft_dpt.ry() = pt.y();
					m->bottomright_dpt.rx() = pt.x();
				} else if (m->rect_handle == RectHandle::BottomLeft) {
					m->topleft_dpt.rx() = pt.x();
					m->bottomright_dpt.ry() = pt.y();
				} else if (m->rect_handle == RectHandle::Top) {
					m->topleft_dpt.ry() = pt.y();
				} else if (m->rect_handle == RectHandle::Left) {
					m->topleft_dpt.rx() = pt.x();
				} else if (m->rect_handle == RectHandle::Right) {
					m->bottomright_dpt.rx() = pt.x();
				} else if (m->rect_handle == RectHandle::Bottom) {
					m->bottomright_dpt.ry() = pt.y();
				}
				setRect();
			}
		}
		return true;
	}

	return false;
}

bool MainWindow::onMouseLeftButtonRelase(int x, int y, bool leftbutton)
{
	Tool tool = currentTool();
	if (tool == Tool::Scroll) return false;

	if (tool == Tool::Brush) {
		if (leftbutton) {
			QPointF pos = pointOnDocument(x, y);
			onPenUp(pos.x(), pos.y());
			return true;
		}
	}

	if (tool == Tool::Rect) {

		if (leftbutton) {
			m->topleft_dpt = m->rect_topleft_dpt;
			m->bottomright_dpt = m->rect_bottomright_dpt;
		}
		return true;
	}

	return false;
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
	bool ctrl = (event->modifiers() & Qt::ControlModifier);
	int k = event->key();
	switch (k) {
	case Qt::Key_T:
		if (ctrl) {
			test();
		}
		return;
	case Qt::Key_P:
		if (ctrl) {
			QList<QScreen *> list = QApplication::screens();
			std::vector<QRect> bounds;
			for (int i = 0; i < list.size(); i++) {
				bounds.push_back(list[i]->geometry());
			}
			if (!bounds.empty()) {
				QElapsedTimer t;
				t.start();
				QImage im;
				{
					QPixmap pm = list.front()->grabWindow(0);
					im = QImage(pm.width(), pm.height(), QImage::Format_RGBA8888);
					im.fill(Qt::transparent);

					QPainter pr(&im);
					for (int i = 0; i < (int)bounds.size(); i++) {
						QRect r = bounds[i];
						pr.drawPixmap(r, pm, r);
					}
				}
				setImage(im, true);
				qDebug() << QString("%1ms").arg(t.elapsed());
			}
		}
		return;
	}
	QMainWindow::keyPressEvent(event);
}

void MainWindow::changeTool(Tool tool)
{
	m->current_tool = tool;

	struct Button {
		Tool tool;
		QToolButton *button;
	};

	Button buttons[] = {
		Tool::Scroll, ui->toolButton_scroll,
		Tool::Brush, ui->toolButton_brush,
		Tool::Rect, ui->toolButton_rect,
	};

	int n = sizeof(buttons) / sizeof(*buttons);
	for (int i = 0; i < n; i++) {
		bool f = (buttons[i].tool == m->current_tool);
		buttons[i].button->setChecked(f);
	}
}

MainWindow::Tool MainWindow::currentTool() const
{
	return m->current_tool;
}

void MainWindow::on_toolButton_scroll_clicked()
{
	changeTool(Tool::Scroll);
}

void MainWindow::on_toolButton_brush_clicked()
{
	changeTool(Tool::Brush);
}

void MainWindow::on_toolButton_rect_clicked()
{
	changeTool(Tool::Rect);
}

SelectionOutlineBitmap MainWindow::renderSelectionOutlineBitmap(bool *abort)
{
	return ui->widget_image_view->renderSelectionOutlineBitmap(abort);
}

void MainWindow::on_action_edit_copy_triggered()
{
	QImage image = selectedImage();
	QApplication::clipboard()->setImage(image);
}

void MainWindow::on_action_new_triggered()
{
	NewDialog dlg(this);
	if (dlg.exec() == QDialog::Accepted) {
		QSize sz = dlg.imageSize();
		if (dlg.from() == NewDialog::From::New) {
			QImage image(sz.width(), sz.height(), QImage::Format_RGBA8888);
			setImage(image, true);
			return;
		}
		if (dlg.from() == NewDialog::From::Clipboard) {
			QImage image = selectedImage();
			setImage(image, true);
			return;
		}
	}
}

void MainWindow::test()
{
}



