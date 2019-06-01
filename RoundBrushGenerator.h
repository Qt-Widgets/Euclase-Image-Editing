#ifndef ROUNDBRUSHGENERATOR_H
#define ROUNDBRUSHGENERATOR_H

class RoundBrushGenerator {
private:
public:
	float radius;
	float blur;
	float mul;
public:
	RoundBrushGenerator(double size, double softness);
	double level(double x, double y);
};


#endif // ROUNDBRUSHGENERATOR_H
