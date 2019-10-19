#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "Document.h"
#include "SelectionOutlineRenderer.h"

#include <QMainWindow>

class Brush;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
	Q_OBJECT
public:
	enum class Tool {
		Scroll,
		Brush,
		Rect,
	};
	enum RectHandle {
		None,
		Top,
		Left,
		Right,
		Bottom,
		TopLeft,
		TopRight,
		BottomLeft,
		BottomRight,
	};
private:
	Ui::MainWindow *ui;

	struct Private;
	Private *m;

	void setImage(const QImage &image, bool fitview);
	void setImage(QByteArray const &ba, bool fitview);

	enum class Operation {
		PaintToCurrentLayer,
	};
	void paintLayer(Operation op, const Document::Layer &layer);
	void changeSelection(Document::SelectionOperation op);

	void drawBrush(bool one);
	void test();
	void updateImageView();
	void updateSelectionOutline();
	void setColorRed(int value);
	void setColorGreen(int value);
	void setColorBlue(int value);
	void setColorHue(int value);
	void setColorSaturation(int value);
	void setColorValue(int value);
	QImage renderFilterTargetImage();
	void onSelectionChanged();
	void clearSelection();
	QImage selectedImage() const;
	MainWindow::RectHandle rectHitTest(const QPoint &pt) const;
	QPointF pointOnDocument(int x, int y) const;
	QPointF mapFromViewportToDocument(const QPointF &pt) const;
	QPointF mapFromDocumentToViewport(const QPointF &pt) const;
	void setRect();
protected:
	void keyPressEvent(QKeyEvent *event);
public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

	Document *document();
	Document const *document() const;

	QMutex *synchronizer() const;

	void fitView();
	QImage renderImage(const QRect &r, bool quickmask, bool *abort) const;
	QRect selectionRect() const;
	void openFile(const QString &path);
	int documentWidth() const;
	int documentHeight() const;
	QColor foregroundColor() const;
	const Brush &currentBrush() const;
	void changeTool(Tool tool);
	MainWindow::Tool currentTool() const;
	SelectionOutlineBitmap renderSelectionOutline(bool *abort) const;
	SelectionOutlineBitmap renderSelectionOutlineBitmap(bool *abort);
public slots:
	void setForegroundColor(QColor const &color);
	void setCurrentBrush(const Brush &brush);
	void onPenDown(double x, double y);
	void onPenStroke(double x, double y);
	void onPenUp(double x, double y);
	bool onMouseLeftButtonPress(int x, int y);
	bool onMouseMove(int x, int y, bool leftbutton);
	bool onMouseLeftButtonRelase(int x, int y, bool leftbutton);
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
	void on_verticalScrollBar_valueChanged(int value);
	void on_horizontalSlider_size_valueChanged(int value);
	void on_horizontalSlider_softness_valueChanged(int value);
	void on_spinBox_brush_size_valueChanged(int value);
	void on_spinBox_brush_softness_valueChanged(int value);
	void on_horizontalSlider_rgb_r_valueChanged(int value);
	void on_horizontalSlider_rgb_g_valueChanged(int value);
	void on_horizontalSlider_rgb_b_valueChanged(int value);
	void on_spinBox_rgb_r_valueChanged(int value);
	void on_spinBox_rgb_g_valueChanged(int value);
	void on_spinBox_rgb_b_valueChanged(int value);
	void on_horizontalSlider_hsv_h_valueChanged(int value);
	void on_horizontalSlider_hsv_s_valueChanged(int value);
	void on_horizontalSlider_hsv_v_valueChanged(int value);
	void on_spinBox_hsv_h_valueChanged(int value);
	void on_spinBox_hsv_s_valueChanged(int value);
	void on_spinBox_hsv_v_valueChanged(int value);
	void on_toolButton_scroll_clicked();
	void on_toolButton_brush_clicked();
	void on_toolButton_rect_clicked();
	void on_action_edit_copy_triggered();
	void on_action_new_triggered();
};

#endif // MAINWINDOW_H
