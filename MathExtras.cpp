#include <MathExtras.h>

float lerpf(float a, float b, float t)
{
	return a + t * (b - a);
}

float invlerpf(float a, float b, float v)
{
	return (v - a) / (b - a);
}

float remapf(float iMin, float iMax, float oMin, float oMax, float v)
{
	float t = invlerpf(iMin, iMax, v);
	return lerpf(oMin, oMax, t);
}
