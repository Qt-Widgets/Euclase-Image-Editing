#ifndef ROUNDBRUSHGENERATOR_H
#define ROUNDBRUSHGENERATOR_H

class Brush {
public:
	double size = 200;
	double softness = 1;
};

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
