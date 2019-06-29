#include "AlphaBlend.h"

AlphaBlend::AlphaBlend()
{

}




int16_t AlphaBlend::fixed_t::lut_mul_[65536] = {1};

const int16_t *AlphaBlend::fixed_t::lut_mul()
{
	if (lut_mul_[0]) {
		for (int i = 0; i < 65536; i++) {

		}
	}
	return lut_mul_;
}
