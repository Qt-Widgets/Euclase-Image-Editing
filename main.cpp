#include "MainWindow.h"
#include "MyApplication.h"
#include "main.h"
#include "MiraCL.h"
#include "AlphaBlend.h"
#include "SelectionOutlineRenderer.h"
#include "ImageViewRenderer.h"

#include <QDebug>

int main(int argc, char *argv[])
{
	MyApplication a(argc, argv);

#if USE_OPENCL
	getCL()->open();
#endif

	qRegisterMetaType<RenderedImage>("RenderedImage");
	qRegisterMetaType<SelectionOutlineBitmap>("SelectionOutlineBitmap");

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

