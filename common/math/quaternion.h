#ifndef LW_QUATERNION_H
#define LW_QUATERNION_H

#include "vector.h"

class Matrix4x4;
class EAngle;

class Quaternion
{
public:
						Quaternion ();
						Quaternion (float x, float y, float z, float w);
						Quaternion (const Quaternion& q);
						Quaternion (const Matrix4x4& m);

public:
	EAngle				GetAngles() const;
	void				SetAngles(const EAngle& a);

	void				SetRotation(float flAngle, const Vector& v);

	void				Normalize();

	Quaternion			operator*(const Quaternion& q);
	const Quaternion&	operator*=(const Quaternion& q);

public:
	float x;
	float y;
	float z;
	float w;
};

#endif
