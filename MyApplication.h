#ifndef MYAPPLICATION_H
#define MYAPPLICATION_H

#include <QApplication>

#include "MiraCL.h"

class MyApplication : public QApplication
{
private:
#if USE_OPENCL
	MiraCL cl;
#endif
public:
	MyApplication(int &argc, char **argv, int = ApplicationFlags);

#if USE_OPENCL
	MiraCL *getCL()
	{
		return &cl;
	}
#endif
};

extern MyApplication *theApp;

#endif // MYAPPLICATION_H
