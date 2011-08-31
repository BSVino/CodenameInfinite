#ifndef LW_VECTOR_H
#define LW_VECTOR_H

#include <math.h>

// for size_t
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265f
#endif

template <class unit_t>
class TemplateVector2D;

template <class unit_t>
class TemplateVector
{
public:
			TemplateVector();
			TemplateVector(class Color c);
			TemplateVector(unit_t x, unit_t y, unit_t z);
			TemplateVector(unit_t* xyz);

			// Conversions
			TemplateVector(const TemplateVector<float>& v);
			TemplateVector(const TemplateVector<double>& v);
			TemplateVector(const class TemplateVector2D<float>& v);
			TemplateVector(const class TemplateVector2D<double>& v);

public:
	TemplateVector<unit_t>	operator-(void) const;

	TemplateVector<unit_t>	operator+(const TemplateVector<unit_t>& v) const;
	TemplateVector<unit_t>	operator-(const TemplateVector<unit_t>& v) const;
	TemplateVector<unit_t>	operator*(unit_t s) const;
	TemplateVector<unit_t>	operator/(unit_t s) const;

	void	operator+=(const TemplateVector<unit_t> &v);
	void	operator-=(const TemplateVector<unit_t> &v);
	void	operator*=(unit_t s);
	void	operator/=(unit_t s);

	TemplateVector<unit_t>	operator*(const TemplateVector<unit_t>& v) const;

	friend TemplateVector<unit_t> operator*( unit_t f, const TemplateVector<unit_t>& v )
	{
		return TemplateVector( v.x*f, v.y*f, v.z*f );
	}

	friend TemplateVector<unit_t> operator/( unit_t f, const TemplateVector<unit_t>& v )
	{
		return TemplateVector( f/v.x, f/v.y, f/v.z );
	}

	unit_t	Length() const;
	unit_t	LengthSqr() const;
	unit_t	Length2D() const;
	unit_t	Length2DSqr() const;
	void	Normalize();
	TemplateVector<unit_t>	Normalized() const;

	unit_t	Distance(const TemplateVector<unit_t>& v) const;
	unit_t	DistanceSqr(const TemplateVector<unit_t>& v) const;

	unit_t	Dot(const TemplateVector<unit_t>& v) const;
	TemplateVector<unit_t>	Cross(const TemplateVector<unit_t>& v) const;

	operator unit_t*()
	{
		return(&x);
	}

	operator const unit_t*() const
	{
		return(&x);
	}

	unit_t	operator[](int i) const;
	unit_t&	operator[](int i);

	unit_t	operator[](size_t i) const;
	unit_t&	operator[](size_t i);

	unit_t	x, y, z;
};

typedef TemplateVector<float> Vector;
typedef TemplateVector<double> DoubleVector;

#include <color.h>

template <class unit_t>
inline TemplateVector<unit_t>::TemplateVector()
	: x(0), y(0), z(0)
{
}

template <class unit_t>
inline TemplateVector<unit_t>::TemplateVector(Color c)
{
	x = (float)c.r()/255.0f;
	y = (float)c.g()/255.0f;
	z = (float)c.b()/255.0f;
}

template <class unit_t>
inline TemplateVector<unit_t>::TemplateVector(unit_t X, unit_t Y, unit_t Z)
	: x(X), y(Y), z(Z)
{
}

template <class unit_t>
inline TemplateVector<unit_t>::TemplateVector(unit_t* xyz)
	: x(*xyz), y(*(xyz+1)), z(*(xyz+2))
{
}

template <class unit_t>
inline TemplateVector<unit_t>::TemplateVector(const TemplateVector<float>& v)
	: x((unit_t)v.x), y((unit_t)v.y), z((unit_t)v.z)
{
}

template <class unit_t>
inline TemplateVector<unit_t>::TemplateVector(const TemplateVector<double>& v)
	: x((unit_t)v.x), y((unit_t)v.y), z((unit_t)v.z)
{
}

template <class unit_t>
inline TemplateVector<unit_t> TemplateVector<unit_t>::operator-() const
{
	return TemplateVector(-x, -y, -z);
}

template <class unit_t>
inline TemplateVector<unit_t> TemplateVector<unit_t>::operator+(const TemplateVector<unit_t>& v) const
{
	return TemplateVector(x+v.x, y+v.y, z+v.z);
}

template <class unit_t>
inline TemplateVector<unit_t> TemplateVector<unit_t>::operator-(const TemplateVector<unit_t>& v) const
{
	return TemplateVector(x-v.x, y-v.y, z-v.z);
}

template <class unit_t>
inline TemplateVector<unit_t> TemplateVector<unit_t>::operator*(unit_t s) const
{
	return TemplateVector(x*s, y*s, z*s);
}

template <class unit_t>
inline TemplateVector<unit_t> TemplateVector<unit_t>::operator/(unit_t s) const
{
	return TemplateVector(x/s, y/s, z/s);
}

template <class unit_t>
inline void TemplateVector<unit_t>::operator+=(const TemplateVector<unit_t>& v)
{
	x += v.x;
	y += v.y;
	z += v.z;
}

template <class unit_t>
inline void TemplateVector<unit_t>::operator-=(const TemplateVector<unit_t>& v)
{
	x -= v.x;
	y -= v.y;
	z -= v.z;
}

template <class unit_t>
inline void TemplateVector<unit_t>::operator*=(unit_t s)
{
	x *= s;
	y *= s;
	z *= s;
}

template <class unit_t>
inline void TemplateVector<unit_t>::operator/=(unit_t s)
{
	x /= s;
	y /= s;
	z /= s;
}

template <class unit_t>
inline TemplateVector<unit_t> TemplateVector<unit_t>::operator*(const TemplateVector<unit_t>& v) const
{
	return TemplateVector(x*v.x, y*v.y, z*v.z);
}

template <class unit_t>
inline unit_t TemplateVector<unit_t>::Length() const
{
	return sqrt(x*x + y*y + z*z);
}

template <class unit_t>
inline unit_t TemplateVector<unit_t>::LengthSqr() const
{
	return x*x + y*y + z*z;
}

template <class unit_t>
inline unit_t TemplateVector<unit_t>::Length2D() const
{
	return sqrt(x*x + z*z);
}

template <class unit_t>
inline unit_t TemplateVector<unit_t>::Length2DSqr() const
{
	return x*x + z*z;
}

template <class unit_t>
inline void TemplateVector<unit_t>::Normalize()
{
	unit_t flLength = Length();
	if (!flLength)
		*this=TemplateVector(0,0,1);
	else
		*this/=flLength;
}

template <class unit_t>
inline TemplateVector<unit_t> TemplateVector<unit_t>::Normalized() const
{
	unit_t flLength = Length();
	if (!flLength)
		return TemplateVector(0,0,1);
	else
		return *this/flLength;
}

template <class unit_t>
inline unit_t TemplateVector<unit_t>::Distance(const TemplateVector<unit_t>& v) const
{
	return (*this - v).Length();
}

template <class unit_t>
inline unit_t TemplateVector<unit_t>::DistanceSqr(const TemplateVector<unit_t>& v) const
{
	return (*this - v).LengthSqr();
}

template <class unit_t>
inline unit_t TemplateVector<unit_t>::Dot(const TemplateVector<unit_t>& v) const
{
	return x*v.x + y*v.y + z*v.z;
}

template <class unit_t>
inline TemplateVector<unit_t> TemplateVector<unit_t>::Cross(const TemplateVector<unit_t>& v) const
{
	return TemplateVector(y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x);
}

template <class unit_t>
inline unit_t& TemplateVector<unit_t>::operator[](int i)
{
	return (&x)[i];
}

template <class unit_t>
inline unit_t TemplateVector<unit_t>::operator[](int i) const
{
	return (&x)[i];
}

template <class unit_t>
inline unit_t& TemplateVector<unit_t>::operator[](size_t i)
{
	return (&x)[i];
}

template <class unit_t>
inline unit_t TemplateVector<unit_t>::operator[](size_t i) const
{
	return (&x)[i];
}

// Euler angles
class EAngle
{
public:
			EAngle();
			EAngle(float x, float y, float z);
			EAngle(float* xyz);

	EAngle	operator+(const EAngle& v) const;
	EAngle	operator-(const EAngle& v) const;

	EAngle	operator*(float f) const;
	EAngle	operator/(float f) const;

	operator float*()
	{
		return(&p);
	}

	float	p, y, r;
};

inline EAngle::EAngle()
	: p(0), y(0), r(0)
{
}

inline EAngle::EAngle(float P, float Y, float R)
	: p(P), y(Y), r(R)
{
}

inline EAngle::EAngle(float* pyr)
	: p(*pyr), y(*(pyr+1)), r(*(pyr+2))
{
}

inline EAngle EAngle::operator+(const EAngle& v) const
{
	return EAngle(p+v.p, y+v.y, r+v.r);
}

inline EAngle EAngle::operator-(const EAngle& v) const
{
	return EAngle(p-v.p, y-v.y, r-v.r);
}

inline EAngle EAngle::operator*(float f) const
{
	return EAngle(p*f, y*f, r*f);
}

inline EAngle EAngle::operator/(float f) const
{
	return EAngle(p/f, y/f, r/f);
}

inline Vector AngleVector(const EAngle& a)
{
	Vector vecResult;

	float p = (float)(a.p * (M_PI*2 / 360));
	float y = (float)(a.y * (M_PI*2 / 360));
	float r = (float)(a.r * (M_PI*2 / 360));

	float sp = sin(p);
	float cp = cos(p);
	float sy = sin(y);
	float cy = cos(y);

	vecResult.x = cp*cy;
	vecResult.y = sp;
	vecResult.z = cp*sy;

	return vecResult;
}

inline void AngleVectors(const EAngle& a, Vector* pvecF, Vector* pvecR, Vector* pvecU)
{
	float p = (float)(a.p * (M_PI*2 / 360));
	float y = (float)(a.y * (M_PI*2 / 360));
	float r = 0;

	float sp = sin(p);
	float cp = cos(p);
	float sy = sin(y);
	float cy = cos(y);
	float sr = 0;
	float cr = 0;

	if (pvecR || pvecU)
	{
		r = (float)(a.r * (M_PI*2 / 360));
		sr = sin(r);
		cr = cos(r);
	}

	if (pvecF)
	{
		pvecF->x = cp*cy;
		pvecF->y = sp;
		pvecF->z = cp*sy;
	}

	if (pvecR)
	{
		pvecR->x = -sr*sp*cy + cr*sy;
		pvecR->y = sr*cp;
		pvecR->z = -sr*sp*sy - cr*cy;
	}

	if (pvecU)
	{
		pvecU->x = cr*sp*cy + sr*sy;
		pvecU->y = -cr*cp;
		pvecU->z = cr*sp*sy - sr*cy;
	}
}

inline EAngle VectorAngles( const Vector& vecForward )
{
	EAngle angReturn(0, 0, 0);

	angReturn.p = atan2(vecForward.y, sqrt(vecForward.x*vecForward.x + vecForward.z*vecForward.z)) * 180/M_PI;
	angReturn.y = atan2(vecForward.z, vecForward.x) * 180/M_PI;

	return angReturn;
}

template <class unit_t>
class TemplateVector2D
{
public:
				TemplateVector2D();
				TemplateVector2D(unit_t x, unit_t y);
				TemplateVector2D(TemplateVector<unit_t> v);

				// Conversions
				TemplateVector2D(const TemplateVector2D<float>& v);
				TemplateVector2D(const TemplateVector2D<double>& v);

public:
	float	LengthSqr() const;

	TemplateVector2D<unit_t>	operator+(const TemplateVector2D<unit_t>& v) const;
	TemplateVector2D<unit_t>	operator-(const TemplateVector2D<unit_t>& v) const;
	TemplateVector2D<unit_t>	operator*(float s) const;
	TemplateVector2D<unit_t>	operator/(float s) const;

	operator float*()
	{
		return(&x);
	}

	operator const float*() const
	{
		return(&x);
	}

	unit_t	x, y;
};

template <class unit_t>
inline TemplateVector<unit_t>::TemplateVector(const TemplateVector2D<float>& v)
	: x((unit_t)v.x), y((unit_t)v.y), z(0)
{
}

template <class unit_t>
inline TemplateVector<unit_t>::TemplateVector(const TemplateVector2D<double>& v)
	: x((unit_t)v.x), y((unit_t)v.y), z(0)
{
}

typedef TemplateVector2D<float> Vector2D;
typedef TemplateVector2D<double> DoubleVector2D;

template <class unit_t>
inline TemplateVector2D<unit_t>::TemplateVector2D()
	: x(0), y(0)
{
}

template <class unit_t>
inline TemplateVector2D<unit_t>::TemplateVector2D(unit_t X, unit_t Y)
	: x(X), y(Y)
{
}

template <class unit_t>
inline TemplateVector2D<unit_t>::TemplateVector2D(TemplateVector<unit_t> v)
	: x(v.x), y(v.y)
{
}

template <class unit_t>
inline TemplateVector2D<unit_t>::TemplateVector2D(const TemplateVector2D<float>& v)
	: x((unit_t)v.x), y((unit_t)v.y)
{
}

template <class unit_t>
inline TemplateVector2D<unit_t>::TemplateVector2D(const TemplateVector2D<double>& v)
	: x((unit_t)v.x), y((unit_t)v.y)
{
}

template <class unit_t>
inline float TemplateVector2D<unit_t>::LengthSqr() const
{
	return x*x + y*y;
}

template <class unit_t>
inline TemplateVector2D<unit_t> TemplateVector2D<unit_t>::operator+(const TemplateVector2D<unit_t>& v) const
{
	return TemplateVector2D(x+v.x, y+v.y);
}

template <class unit_t>
inline TemplateVector2D<unit_t> TemplateVector2D<unit_t>::operator-(const TemplateVector2D<unit_t>& v) const
{
	return TemplateVector2D(x-v.x, y-v.y);
}

template <class unit_t>
inline TemplateVector2D<unit_t> TemplateVector2D<unit_t>::operator*(float s) const
{
	return TemplateVector2D(x*s, y*s);
}

template <class unit_t>
inline TemplateVector2D<unit_t> TemplateVector2D<unit_t>::operator/(float s) const
{
	return TemplateVector2D(x/s, y/s);
}

class Vector4D
{
public:
				Vector4D();
				Vector4D(const Vector& v);
				Vector4D(const Vector& v, float w);
				Vector4D(const class Color& c);
				Vector4D(float x, float y, float z, float w);

public:
	Vector4D	operator+(const Vector4D& v) const;
	Vector4D	operator-(const Vector4D& v) const;

	operator float*()
	{
		return(&x);
	}

	float	x, y, z, w;
};

#include "color.h"

inline Vector4D::Vector4D()
	: x(0), y(0), z(0), w(0)
{
}

inline Vector4D::Vector4D(const Vector& v)
	: x(v.x), y(v.y), z(v.z), w(0)
{
}

inline Vector4D::Vector4D(const Vector& v, float W)
	: x(v.x), y(v.y), z(v.z), w(W)
{
}

inline Vector4D::Vector4D(const Color& c)
	: x(((float)c.r())/255), y(((float)c.g())/255), z(((float)c.b())/255), w(((float)c.a())/255)
{
}

inline Vector4D::Vector4D(float X, float Y, float Z, float W)
	: x(X), y(Y), z(Z), w(W)
{
}

inline Vector4D Vector4D::operator+(const Vector4D& v) const
{
	return Vector4D(x+v.x, y+v.y, z+v.z, w+v.w);
}

inline Vector4D Vector4D::operator-(const Vector4D& v) const
{
	return Vector4D(x-v.x, y-v.y, z-v.z, w-v.w);
}

#endif
