
#ifndef MEDIAN_H_
#define MEDIAN_H_

#include <QImage>

QImage filter_median(QImage image, int radius);
QImage filter_maximize(QImage image, int radius);
QImage filter_minimize(QImage image, int radius);

#endif

