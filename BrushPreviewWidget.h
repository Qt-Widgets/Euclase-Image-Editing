#ifndef BRUSHPREVIEWWIDGET_H
#define BRUSHPREVIEWWIDGET_H

#include "MainWindow.h"
#include "MiraCL.h"

#include <QWidget>

class BrushPreviewWidget : public QWidget {
	Q_OBJECT
private:
	double size = 200;
	double softness = 1;
#if USE_OPENCL
	MiraCL *getCL();
	MiraCL::Program prog;
#endif
	MainWindow *mainwindow();
public:
	explicit BrushPreviewWidget(QWidget *parent = 0);

	void setBrushSize(double v);
	void setBrushSoftness(double percent);
	double brushSize() const;
	double brushSoftness() const;
signals:

public slots:

	// QWidget interface
protected:
	void paintEvent(QPaintEvent *);
};



#endif // BRUSHPREVIEWWIDGET_H
