#pragma once

// Linear interpolation
float lerpf(float a, float b, float t);

// Inverse linear interpolation
float invlerpf(float a, float b, float v);

// Remap
float remapf(float iMin, float iMax, float oMin, float oMax, float v);
