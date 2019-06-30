
QT       += core gui widgets svg

TARGET = Euclase
TEMPLATE = app
CONFIG += c++11

DESTDIR = $$PWD/_bin

#INCLUDEPATH += "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v7.5/include"
#LIBS += "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v7.5/lib/Win32/OpenCL.lib"

SOURCES += main.cpp\
	AlphaBlend.cpp \
	Document.cpp \
        MainWindow.cpp \
    BrushPreviewWidget.cpp \
    MiraCL.cpp \
    MyWidget.cpp \
    MyApplication.cpp \
    HueWidget.cpp \
    SaturationBrightnessWidget.cpp \
	antialias.cpp \
	median.cpp \
    misc.cpp \
    RoundBrushGenerator.cpp \
    ResizeDialog.cpp \
    ImageViewWidget.cpp \
    MemoryReader.cpp \
    Photoshop.cpp \
    charvec.cpp \
    joinpath.cpp \
	resize.cpp

HEADERS  += MainWindow.h \
    AlphaBlend.h \
    BrushPreviewWidget.h \
    Document.h \
    MiraCL.h \
    MyWidget.h \
    antialias.h \
    main.h \
    MyApplication.h \
    HueWidget.h \
    SaturationBrightnessWidget.h \
    median.h \
    misc.h \
    RoundBrushGenerator.h \
    ResizeDialog.h \
    ImageViewWidget.h \
    MemoryReader.h \
    Photoshop.h \
    charvec.h \
    joinpath.h \
    resize.h

FORMS    += MainWindow.ui \
    ResizeDialog.ui

RESOURCES += \
    resources.qrc
