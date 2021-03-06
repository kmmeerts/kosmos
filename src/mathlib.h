#ifndef KOSMOS_MATHLIB_H
#define KOSMOS_MATHLIB_H

#ifndef M_PI
#define M_PI 3.141592653589793238462643
#endif
#ifndef M_TWO_PI
#define M_TWO_PI 6.283185307179586476925287
#endif
#ifndef M_SQRT2
#define M_SQRT2 1.4142135623730950488016887242096980785696718753769480731766
#endif
#ifndef GRAV_CONST
#define GRAV_CONST 6.67384e-11
#endif

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define ABS(x) ((a) > 0 ? (a) : -(a))
#define SQUARE(x) ((x) * (x))
#define CUBE(x) ((x) * (x) * (x))
#define RAD(x) ((x) * M_TWO_PI / 360.0)

#include "quaternion.h"
#include "vector.h"
#include "matrix.h"

#endif
