#ifndef MYWIDGET_H
#define MYWIDGET_H

#include <QWidget>

#include "MiraCL.h"

class MyWidget : public QWidget
{
private:
	QImage image;

#if USE_OPENCL
	MiraCL::Program prog;
public:
	MiraCL *getCL();
#endif
public:
	MyWidget(QWidget *parent);

	// QWidget interface
	~MyWidget();
protected:
	virtual void paintEvent(QPaintEvent *);

	// QWidget interface
protected:
	virtual void resizeEvent(QResizeEvent *);
};

#endif // MYWIDGET_H
