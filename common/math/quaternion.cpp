#include "quaternion.h"

#include "vector.h"
#include "matrix.h"

Quaternion::Quaternion()
{
	x = 1;
	y = 0;
	z = 0;
	w = 0;
}

Quaternion::Quaternion(float X, float Y, float Z, float W)
{
	x = X;
	y = Y;
	z = Z;
	w = W;
}

Quaternion::Quaternion(const Quaternion& q)
{
	x = q.x;
	y = q.y;
	z = q.z;
	w = q.w;
}

Quaternion::Quaternion(const Matrix4x4& m)
{
	// Algorithm in Ken Shoemake's article in 1987 SIGGRAPH course notes
	// article "Quaternion Calculus and Fast Animation".

	float flTrace = m.Trace();
	float flRoot;

	if ( flTrace > 0 )
	{
		// |w| > 1/2, may as well choose w > 1/2
		flRoot = sqrt(flTrace + 1);  // 2w

		w = flRoot/2;

		flRoot = 0.5f/flRoot;  // 1/(4w)
		x = (m.m[2][1]-m.m[1][2])*flRoot;
		y = (m.m[0][2]-m.m[2][0])*flRoot;
		z = (m.m[1][0]-m.m[0][1])*flRoot;
	}
	else
	{
		const int NEXT[3] = { 1, 2, 0 };

		// |w| <= 1/2
		int i = 0;
		if ( m.m[1][1] > m.m[0][0] )
			i = 1;
		if ( m.m[2][2] > m.m[i][i] )
			i = 2;
		int j = NEXT[i];
		int k = NEXT[j];

		flRoot = sqrt(m.m[i][i]-m.m[j][j]-m.m[k][k]+1);
		float* apflQuat[3] = { &x, &y, &z };
		*apflQuat[i] = flRoot/2;
		flRoot = 0.5f/flRoot;
		w = (m.m[k][j]-m.m[j][k])*flRoot;
		*apflQuat[j] = (m.m[j][i]+m.m[i][j])*flRoot;
		*apflQuat[k] = (m.m[k][i]+m.m[i][k])*flRoot;
	}
}

EAngle Quaternion::GetAngles() const
{
	float sqx = x*x;
	float sqy = y*y;
	float sqz = z*z;
	float sqw = w*w;
	
	EAngle angResult;
	angResult.p = atan2(2*(y*z + x*w), (-sqx-sqy+sqz+sqw)) *180/M_PI;
	angResult.y = asin(-2*(x*z - y*w)) *180/M_PI;
	angResult.r = atan2(2*(x*y + z*w), (sqx-sqy-sqz+sqw)) *180/M_PI;
	return angResult;
}

void Quaternion::SetAngles(const EAngle& a)
{
	float c1 = cos(a.y/2 *M_PI/180);
	float s1 = sin(a.y/2 *M_PI/180);
	float c2 = cos(a.p/2 *M_PI/180);
	float s2 = sin(a.p/2 *M_PI/180);
	float c3 = cos(a.r/2 *M_PI/180);
	float s3 = sin(a.r/2 *M_PI/180);

	float c1c2 = c1*c2;
	float s1s2 = s1*s2;

	x = c1c2*s3 + s1s2*c3;
	y = s1*c2*c3 + c1*s2*s3;
	z = c1*s2*c3 - s1*c2*s3;
	w = c1c2*c3 - s1s2*s3;
}

void Quaternion::SetRotation(float flAngle, const Vector& v)
{
	float flHalfAngle = flAngle/2;
	float flSin = sin(flHalfAngle);

	x = flSin*v.x;
	y = flSin*v.y;
	z = flSin*v.z;

	w = cos(flHalfAngle);
}

void Quaternion::Normalize()
{
	float m = sqrt(w*w + x*x + y*y + z*z);
	w = w/m;
	x = x/m;
	y = y/m;
	z = z/m;
}

Quaternion Quaternion::operator*(const Quaternion& q)
{
	Quaternion r;
	r.w = w*q.w - x*q.x - y*q.y - z*q.z;
	r.x = w*q.x + x*q.w + y*q.z - z*q.y;
	r.y = w*q.y + y*q.w + z*q.x - x*q.z;
	r.z = w*q.z + z*q.w + x*q.y - y*q.x;
    return r;
}

const Quaternion& Quaternion::operator*=(const Quaternion& q)
{
	w = w*q.w - x*q.x - y*q.y - z*q.z;
	x = w*q.x + x*q.w + y*q.z - z*q.y;
	y = w*q.y + y*q.w + z*q.x - x*q.z;
	z = w*q.z + z*q.w + x*q.y - y*q.x;
    return *this;
}
