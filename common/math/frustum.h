#ifndef LW_FRUSTUM_H
#define LW_FRUSTUM_H

#include "plane.h"
#include "matrix.h"

// If you change this, also change TouchesSphereSidesOnly
#define FRUSTUM_NEAR	0
#define FRUSTUM_FAR		1
#define FRUSTUM_LEFT	2
#define FRUSTUM_RIGHT	3
#define FRUSTUM_UP		4
#define FRUSTUM_DOWN	5

class Frustum
{
public:
	void		CreateFrom(const Matrix4x4& m);
	bool		TouchesSphere(const Vector& vecCenter, float flRadius);
	bool		TouchesSphereSidesOnly(const Vector& vecCenter, float flRadius);
	bool		ContainsSphereSidesOnly(const Vector& vecCenter, float flRadius);

public:
	Plane		p[6];
};

#endif
