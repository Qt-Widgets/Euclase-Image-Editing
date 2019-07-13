#include "euclase.h"

double euclase::cubicBezierPoint(double p0, double p1, double p2, double p3, double t)
{
	double u = 1 - t;
	return p0 * u * u * u + p1 * u * u * t * 3 + p2 * u * t * t * 3 + p3 * t * t * t;
}

double euclase::cubicBezierGradient(double p0, double p1, double p2, double p3, double t)
{
	return 0 - p0 * (t * t - t * 2 + 1) + p1 * (t * t * 3 - t * 4 + 1) - p2 * (t * t * 3 - t * 2) + p3 * t * t;
}

static void cubicBezierSplit(double *p0, double *p1, double *p2, double *p3, double *p4, double *p5, double t)
{
	double p = euclase::cubicBezierPoint(*p0, *p1, *p2, *p3, t);
	double q = euclase::cubicBezierGradient(*p0, *p1, *p2, *p3, t);
	double u = 1 - u;
	*p4 = p + q * u;
	*p5 = *p3 + (*p2 - *p3) * u;
	*p1 = *p0 + (*p1 - *p0) * t;
	*p2 = p - q * t;
	*p3 = p;
}

QPointF euclase::cubicBezierPoint(QPointF &p0, QPointF &p1, QPointF &p2, QPointF &p3, double t)
{
	double x = cubicBezierPoint(p0.x(), p1.x(), p2.x(), p3.x(), t);
	double y = cubicBezierPoint(p0.y(), p1.y(), p2.y(), p3.y(), t);
	return QPointF(x, y);
}

void euclase::cubicBezierSplit(QPointF *p0, QPointF *p1, QPointF *p2, QPointF *p3, QPointF *q0, QPointF *q1, QPointF *q2, QPointF *q3, double t)
{
	double p4, p5;
	*q3 = *p3;
	::cubicBezierSplit(&p0->rx(), &p1->rx(), &p2->rx(), &p3->rx(), &p4, &p5, t);
	q1->rx() = p4;
	q2->rx() = p5;
	::cubicBezierSplit(&p0->ry(), &p1->ry(), &p2->ry(), &p3->ry(), &p4, &p5, t);
	q1->ry() = p4;
	q2->ry() = p5;
	*q0 = *p3;
}
