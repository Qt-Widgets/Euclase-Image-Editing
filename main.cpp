#include "MainWindow.h"
#include "MyApplication.h"
#include "main.h"
#include "MiraCL.h"

MyApplication *theApp = nullptr;

#include "AlphaBlend.h"

#include <QDebug>

int main(int argc, char *argv[])
{
	using fixed_t = AlphaBlend::fixed_t;

	{
		fixed_t a((float)0);
		fixed_t b((float)1);
		fixed_t c = a * b;
		qDebug() << (uint8_t)c;
	}

	MyApplication a(argc, argv);
	theApp = &a;

#if USE_OPENCL
	getCL()->open();
#endif

	MainWindow w;
	w.show();

	return a.exec();
}

#if USE_OPENCL
MiraCL *getCL()
{
	return theApp->getCL();
}
#endif

