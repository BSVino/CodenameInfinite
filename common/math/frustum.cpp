#include "frustum.h"

void Frustum::CreateFrom(const Matrix4x4& m)
{
	p[FRUSTUM_RIGHT].n.x = m.m[0][3] - m.m[0][0];
	p[FRUSTUM_RIGHT].n.y = m.m[1][3] - m.m[1][0];
	p[FRUSTUM_RIGHT].n.z = m.m[2][3] - m.m[2][0];
	p[FRUSTUM_RIGHT].d = m.m[3][3] - m.m[3][0];

	p[FRUSTUM_LEFT].n.x = m.m[0][3] + m.m[0][0];
	p[FRUSTUM_LEFT].n.y = m.m[1][3] + m.m[1][0];
	p[FRUSTUM_LEFT].n.z = m.m[2][3] + m.m[2][0];
	p[FRUSTUM_LEFT].d = m.m[3][3] + m.m[3][0];

	p[FRUSTUM_DOWN].n.x = m.m[0][3] + m.m[0][1];
	p[FRUSTUM_DOWN].n.y = m.m[1][3] + m.m[1][1];
	p[FRUSTUM_DOWN].n.z = m.m[2][3] + m.m[2][1];
	p[FRUSTUM_DOWN].d = m.m[3][3] + m.m[3][1];

	p[FRUSTUM_UP].n.x = m.m[0][3] - m.m[0][1];
	p[FRUSTUM_UP].n.y = m.m[1][3] - m.m[1][1];
	p[FRUSTUM_UP].n.z = m.m[2][3] - m.m[2][1];
	p[FRUSTUM_UP].d = m.m[3][3] - m.m[3][1];

	p[FRUSTUM_FAR].n.x = m.m[0][3] - m.m[0][2];
	p[FRUSTUM_FAR].n.y = m.m[1][3] - m.m[1][2];
	p[FRUSTUM_FAR].n.z = m.m[2][3] - m.m[2][2];
	p[FRUSTUM_FAR].d = m.m[3][3] - m.m[3][2];

	p[FRUSTUM_NEAR].n.x = m.m[0][3] + m.m[0][2];
	p[FRUSTUM_NEAR].n.y = m.m[1][3] + m.m[1][2];
	p[FRUSTUM_NEAR].n.z = m.m[2][3] + m.m[2][2];
	p[FRUSTUM_NEAR].d = m.m[3][3] + m.m[3][2];

	// Normalize all plane normals
	for(int i = 0; i < 6; i++)
		p[i].Normalize();
}

bool Frustum::TouchesSphere(const Vector& vecCenter, float flRadius)
{
	for (size_t i = 0; i < 6; i++)
	{
		float flDistance = p[i].n.x*vecCenter.x + p[i].n.y*vecCenter.y + p[i].n.z*vecCenter.z + p[i].d;
		if (flDistance + flRadius < 0)
			return false;
	}

	return true;
}

bool Frustum::TouchesSphereSidesOnly(const Vector& vecCenter, float flRadius)
{
	for (size_t i = 2; i < 6; i++)
	{
		float flDistance = p[i].n.x*vecCenter.x + p[i].n.y*vecCenter.y + p[i].n.z*vecCenter.z + p[i].d;
		if (flDistance + flRadius < 0)
			return false;
	}

	return true;
}

bool Frustum::ContainsSphereSidesOnly(const Vector& vecCenter, float flRadius)
{
	for (size_t i = 2; i < 6; i++)
	{
		float flDistance = p[i].n.x*vecCenter.x + p[i].n.y*vecCenter.y + p[i].n.z*vecCenter.z + p[i].d;
		if (flDistance + flRadius < 0)
			return false;

		if (flDistance < flRadius || -flDistance < flRadius)
			return false;
	}

	return true;
}
