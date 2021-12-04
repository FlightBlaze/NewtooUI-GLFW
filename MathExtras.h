#pragma once

// Linear interpolation
float lerpf(float a, float b, float t);

// Inverse linear interpolation
float invlerpf(float a, float b, float v);

// Remap
float remapf(float iMin, float iMax, float oMin, float oMax, float v);

// Clamp
float clampf(float vmin, float value, float vmax);

#include <glm/glm.hpp>

bool isPointInTriange(glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec2 point);

static const float NoIntersection = 99999;

float raySegmentIntersection(glm::vec2 rayOrigin, glm::vec2 rayDirection,
                             glm::vec2 lineStart, glm::vec2 lineEnd);

glm::mat4 toMatrix3D(glm::mat3 mat2d);

// range from 0 to 360
// For example, -10 = 350
float degreesInRange(const float deg);
