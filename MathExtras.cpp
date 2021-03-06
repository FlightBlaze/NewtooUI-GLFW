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

float clampf(float vmin, float value, float vmax) {
    return fmaxf(vmin, fminf(vmax, value));
}

// Point in triangle test with barycentric coordinate system
// http://totologic.blogspot.com/2014/01/accurate-point-in-triangle-test.html
// https://blackpawn.com/texts/pointinpoly/

bool barycentricIsPointInTriangle(glm::vec2 v1, glm::vec2 v2, glm::vec2 v3, glm::vec2 point) {
	float denominator = ((v2.y - v3.y) * (v1.x - v3.x) + (v3.x - v2.x) * (v1.y - v3.y));
	float a = ((v2.y - v3.y) * (point.x - v3.x) + (v3.x - v2.x) * (point.y - v3.y)) / denominator;
	float b = ((v3.y - v1.y) * (point.x - v3.x) + (v1.x - v3.x) * (point.y - v3.y)) / denominator;
	float c = 1 - a - b;

	return 0 <= a && a <= 1 && 0 <= b && b <= 1 && 0 <= c && c <= 1;
}

glm::vec2 perpendicularClockwise(glm::vec2 v)
{
	return glm::vec2(v.y, -v.x);
}

glm::vec2 perpendicularCounterClockwise(glm::vec2 v)
{
	return glm::vec2(-v.y, v.x);
}

//float sameSide(glm::vec2 v1, glm::vec2 v2, glm::vec2 point)
//{
//	return (v2.y - v1.y) * (point.x - v1.x) + (-v2.x + v1.x) * (point.y - v1.y);
//}

bool isPointInTriange(glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec2 point)
{
	//bool side1 = sameSide(a, b, point) >= 0;
	//bool side2 = sameSide(b, c, point) >= 0;
	//bool side3 = sameSide(c, a, point) >= 0;
	//return side1 && side2 && side3;
	return barycentricIsPointInTriangle(a, b, c, point);
}

float crossProduct2D(glm::vec2 a, glm::vec2 b) {
    return a.x * b.y - a.y * b.x;
}

//glm::vec2 rotateVec2(glm::vec2 v, float rad) {
//    return glm::vec2(v.x * sin(rad), v.y * cos(rad));
//}

float raySegmentIntersection(glm::vec2 rayOrigin, glm::vec2 rayDirection,
                             glm::vec2 lineStart, glm::vec2 lineEnd) {
    glm::vec2 v1 = rayOrigin - lineStart;
    glm::vec2 v2 = lineEnd - lineStart;
    glm::vec2 v3 = perpendicularCounterClockwise(rayDirection);

    float dot = glm::dot(v2, v3);
    if (fabsf(dot) < 0.000001f)
        return NoIntersection;

    float t1 = crossProduct2D(v2, v1) / dot;
    float t2 = glm::dot(v1, v3) / dot;

    if (t1 >= 0.0f && (t2 >= 0.0f && t2 <= 1.0f))
        return t1;

    return NoIntersection;
    
//    float radians = atan2f(rayDirection.y, rayDirection.x);
//    glm::vec2 p1 = rotateVec2(lineStart - rayOrigin, radians);
//    glm::vec2 p2 = rotateVec2(lineEnd - rayOrigin, radians);
//
//    glm::vec2 top;
//    glm::vec2 bottom;
//
//    if(p1.y < p2.y) {
//        top = p1;
//        bottom = p2;
//    } else {
//        top = p2;
//        bottom = p1;
//    }
//
//    if(top.y <= 0 && bottom.y >= 0) {
//        float t = 1 - bottom.y / (fabsf(top.y) + fabsf(bottom.y));
//        return glm::mix(top, bottom, t).x; // lerp
//    }
//
//    return NoIntersection;
}

// Convert matrix 3x3 to matrix 4x4
// https://developer.mozilla.org/en-US/docs/Web/CSS/transform-function/matrix()

glm::mat4 toMatrix3D(glm::mat3 mat2d)
{
	float a, b, c, d, tx, ty;
	// a c tx
	// b d ty
	// 0 0 1
	a = mat2d[0][0];
	c = mat2d[0][1];
	tx = mat2d[0][2];
	b = mat2d[1][0];
	d = mat2d[1][1];
	ty = mat2d[1][2];
	// a b 0 0
	// c d 0 0
	// 0 0 1 0
	// tx ty 0 1
	glm::mat4 mat3d = glm::mat4(1.0f);
	mat3d[0][0] = a;
	mat3d[1][0] = b;
	mat3d[0][1] = c;
	mat3d[1][1] = d;
	mat3d[0][3] = tx;
	mat3d[1][3] = ty;
	return mat3d;
}

float degreesInRange(float deg) {
    float rdeg = fabsf(deg);
    float fullCycles = floorf(rdeg / 360.0f);
    rdeg -= fullCycles * 360.0f;
    if (deg < 0.0f)
        rdeg = 360.0f - rdeg;
    return rdeg;
}
