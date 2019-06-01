__kernel void example(uint w, uint h, __global uint *p)
{
	uint x = get_global_id(0);
	uint y = get_global_id(1);
	uint r = x * 256 / w;
	uint g = y * 256 / h;
	uint b = (((x * 16 / w) ^ (y * 16 / h)) & 1) * 255;
	p[x + y * w] = 0xff000000 | (r << 16) | (g << 8) | b;
}

float distance_(float x, float y)
{
	return sqrt(x * x + y * y);
}

__kernel void round_brush_level(int w, int h, int cx, int cy, float radius, float blur, float mul, __global float *p)
{
	int j = get_global_id(0);
	int i = get_global_id(1);
	float x = cx + 0.5;
	float y = cy + 0.5;
	float tx = j + 0.5;
	float ty = i + 0.5;
	x = tx - x;
	y = ty - y;

	float value = 0;
	float d = distance_(x, y);
	if (d > radius) {
		value = 0;
	} else if (d > blur && mul > 0) {
		float t = (d - blur) * mul;
		if (t < 1) {
			float u = 1 - t;
			value = u * u * (u + t * 3);
		}
	} else {
		value = 1;
	}

	p[i * w + j] = value;
}

__kernel void saturation_brightness(uint w, uint h, uint red, uint green, uint blue, __global uint *p)
{
	uint x = get_global_id(0);
	uint y = get_global_id(1);
	float3 rgb = (float3)(red, green, blue);
	float t = x / (w - 1.0);
	float u = 1 - y / (h - 1.0);
	rgb = (255 - (255 - rgb) * t) * u;
	uint r = rgb.x;
	uint g = rgb.y;
	uint b = rgb.z;
	p[x + y * w] = (r << 16) | (g << 8) | b;
}

