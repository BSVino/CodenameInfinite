#ifndef SP_COMMON_H
#define SP_COMMON_H

#include <EASTL/vector.h>

#include <vector.h>
#include <common.h>

typedef enum
{
	SCALE_NONE = 0,
	SCALE_METER,
	SCALE_KILOMETER,	// 1000m
	SCALE_MEGAMETER,	// 1000km
	SCALE_GIGAMETER,	// 1000Mm
	SCALE_TERAMETER,	// 1000Gm - A terameter is about 6.685 astronomical units.
	SCALE_HIGHEST = SCALE_TERAMETER,
} scale_t;

// The scale stack skips SCALE_NONE. SCALE_METER is 0. 
#define SCALESTACK_SIZE (SCALE_HIGHEST)
#define SCALESTACK_INDEX(x) (x-1)

class CScalableVector;
class CScalableMatrix;

class CScalableFloat
{
	friend class CScalableVector;

public:
							CScalableFloat();
							CScalableFloat(float flUnits, scale_t eScale);

public:
	float					GetUnits(scale_t eScale) const;
	bool					IsPositive() const { return m_bPositive; };
	bool					IsNegative() const { return !m_bPositive; };
	bool					IsZero() const;

	CScalableFloat			operator-(void) const;

	CScalableFloat			operator+(const CScalableFloat& u) const;
	CScalableFloat			operator-(const CScalableFloat& u) const;

	void					operator+=(const CScalableFloat& u);
	void					operator-=(const CScalableFloat& u);

	CScalableFloat			operator*(const CScalableFloat& f) const;
	CScalableFloat			operator/(const CScalableFloat& f) const;

	CScalableFloat			operator*(float f) const;
	CScalableFloat			operator/(float f) const;

	void					NormalizeScaleStack();

	bool					operator<(const CScalableFloat& u) const;
	bool					operator>(const CScalableFloat& u) const;

	static float			ConvertUnits(float flUnit, scale_t eFrom, scale_t eTo);

protected:
	bool					m_bPositive;
	float					m_aflScaleStack[SCALESTACK_SIZE];
};

inline CScalableFloat RemapVal(const CScalableFloat& flInput, const CScalableFloat& flInLo, const CScalableFloat& flInHi, const CScalableFloat& flOutLo, const CScalableFloat& flOutHi)
{
	return (((flInput-flInLo) / (flInHi-flInLo)) * (flOutHi-flOutLo)) + flOutLo;
}

class CScalableVector
{
	friend class CScalableMatrix;

public:
							CScalableVector() {};
							CScalableVector(const CScalableFloat& x, const CScalableFloat& y, const CScalableFloat& z);
							CScalableVector(Vector vecUnits, scale_t eScale);

public:
	Vector					GetUnits(scale_t eScale) const;

	bool					IsZero() const;

	CScalableVector			Normalized() const { return CScalableVector(GetUnits(SCALE_METER).Normalized(), SCALE_METER); }
	CScalableFloat			Length() const;
	CScalableFloat			LengthSqr() const;

	CScalableVector			operator-(void) const;

	CScalableVector			operator+(const CScalableVector& v) const;
	CScalableVector			operator-(const CScalableVector& v) const;

	CScalableVector			operator*( const CScalableFloat& f ) const;
	CScalableVector			operator/( const CScalableFloat& f ) const;

	CScalableVector			operator*(float f) const;
	CScalableVector			operator/(float f) const;

	friend CScalableVector operator*( float f, const CScalableVector& v )
	{
		return CScalableVector( v.x*f, v.y*f, v.z*f );
	}

	CScalableFloat			Index(int i) const;
	CScalableFloat&			Index(int i);

	CScalableFloat			operator[](int i) const;
	CScalableFloat&			operator[](int i);

protected:
	CScalableFloat			x, y, z;
};

class CScalableMatrix
{
public:
							CScalableMatrix() { Identity(); }
							CScalableMatrix(const Vector& vecForward, const Vector& vecUp, const Vector& vecRight, const CScalableVector& vecPosition = CScalableVector());

public:
	void					Identity();

	void					SetTranslation(const CScalableVector& m) { mt = m; }
	const CScalableVector&	GetTranslation() const { return mt; }

	void					SetAngles(const EAngle& angDir);
	EAngle					GetAngles() const;

	CScalableMatrix			operator*(const CScalableMatrix& t) const;

	// Transform a vector
	CScalableVector			operator*(const CScalableVector& v) const;

	CScalableVector			TransformNoTranslate(const CScalableVector& v) const;

	void					InvertTR();

	void					SetColumn(int i, const Vector& vecColumn);
	Vector					GetColumn(int i) const;
	Vector					GetForwardVector() const { return GetColumn(0); }
	Vector					GetUpVector() const { return GetColumn(1); }
	Vector					GetRightVector() const { return GetColumn(2); }

	class Matrix4x4			GetUnits(scale_t eScale) const;

public:
	float					m[3][3];
	CScalableVector			mt;
};

inline CScalableVector operator*( Vector v, const CScalableFloat& f )
{
	return CScalableVector( f*v.x, f*v.y, f*v.z );
}

#endif
