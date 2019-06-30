#ifndef BRUSHPREVIEWWIDGET_H
#define BRUSHPREVIEWWIDGET_H

#include "MainWindow.h"
#include "MiraCL.h"
#include "RoundBrushGenerator.h"

#include <QWidget>

class BrushPreviewWidget : public QWidget {
	Q_OBJECT
private:
	Brush brush_;
#if USE_OPENCL
	MiraCL *getCL();
	MiraCL::Program prog;
#endif
	MainWindow *mainwindow();
	void changeBrush();
	double brushSize() const;
	double brushSoftness() const;
public:
	explicit BrushPreviewWidget(QWidget *parent = 0);

	void setBrushSize(double v);
	void setBrushSoftness(double percent);
	void setBrush(const Brush &b);
protected:
	void paintEvent(QPaintEvent *);
};



#endif // BRUSHPREVIEWWIDGET_H
