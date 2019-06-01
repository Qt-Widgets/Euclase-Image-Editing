#include "MainWindow.h"
#include "MyApplication.h"
#include "main.h"
#include "MiraCL.h"

MyApplication *theApp = nullptr;

int main(int argc, char *argv[])
{
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

