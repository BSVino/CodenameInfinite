#ifndef SP_COMMON_H
#define SP_COMMON_H

#include <EASTL/vector.h>

#include <vector.h>
#include <common.h>
#include <quaternion.h>

typedef enum
{
	SCALE_NONE = 0,
	SCALE_MILLIMETER,
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
							CScalableFloat(double flUnits, scale_t eScale);
							CScalableFloat(float flMeters);

public:
	void					Construct(double flUnits, scale_t eScale);

	double					GetUnits(scale_t eScale) const;
	bool					IsPositive() const { return m_bPositive; };
	bool					IsNegative() const { return !m_bPositive; };
	bool					IsZero() const;
	bool					IsZero();

	CScalableFloat			operator-(void) const;

	CScalableFloat			AddSimilar(const CScalableFloat& u) const;
	CScalableFloat			AddDifferent(const CScalableFloat& u) const;

	CScalableFloat			operator+(const CScalableFloat& u) const;
	CScalableFloat			operator-(const CScalableFloat& u) const;

	void					operator+=(const CScalableFloat& u);
	void					operator-=(const CScalableFloat& u);

	CScalableFloat			operator*(const CScalableFloat& f) const;
	CScalableFloat			operator/(const CScalableFloat& f) const;

	CScalableFloat			operator*(float f) const;
	CScalableFloat			operator/(float f) const;

	CScalableFloat			AddMultiple(const CScalableFloat& f, const CScalableFloat& g) const;
	CScalableFloat			AddMultiple(const CScalableFloat& f, const CScalableFloat& g, const CScalableFloat& h) const;

	void					NormalizeStackPosition(int i);
	void					NormalizeRemainder();

	void					CheckSanity();

	bool					operator==(const CScalableFloat& u) const;
	bool					operator<(const CScalableFloat& u) const;
	bool					operator>(const CScalableFloat& u) const;
	bool					operator<(float flMeters) const;
	bool					operator>(float flMeters) const;

	operator double() const;

	static double			ConvertUnits(double flUnit, scale_t eFrom, scale_t eTo);

protected:
	bool					m_bPositive;
	bool					m_bZero;
	short					m_aiScaleStack[SCALESTACK_SIZE];
	double					m_flRemainder;
	double					m_flOverflow;
};

inline CScalableFloat RemapVal(const CScalableFloat& flInput, const CScalableFloat& flInLo, const CScalableFloat& flInHi, const CScalableFloat& flOutLo, const CScalableFloat& flOutHi)
{
	return (((flInput-flInLo) / (flInHi-flInLo)) * (flOutHi-flOutLo)) + flOutLo;
}

inline double RemapVal(const CScalableFloat& flInput, const CScalableFloat& flInLo, const CScalableFloat& flInHi, double flOutLo, double flOutHi)
{
	CScalableFloat f = ((flInput-flInLo) / (flInHi-flInLo));
	return (f.GetUnits(SCALE_METER) * (flOutHi-flOutLo)) + flOutLo;
}

class CScalableVector
{
	friend class CScalableMatrix;

public:
							CScalableVector() {};
							CScalableVector(const CScalableFloat& x, const CScalableFloat& y, const CScalableFloat& z);
							CScalableVector(Vector vecUnits, scale_t eScale);
							CScalableVector(DoubleVector vecUnits, scale_t eScale);
							CScalableVector(Vector vecMeters);

public:
	DoubleVector			GetUnits(scale_t eScale) const;

	bool					IsZero() const;
	bool					IsZero();

	CScalableVector			Normalized() const { return CScalableVector(GetUnits(SCALE_METER).Normalized(), SCALE_METER); }
	Vector					NormalizedVector() const { return GetUnits(SCALE_METER).Normalized(); }
	CScalableFloat			Length() const;
	CScalableFloat			LengthSqr() const;
	CScalableFloat			Dot(const CScalableVector& v) const;
	CScalableVector			Cross(const CScalableVector& v) const;

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

	void					operator+=(const CScalableVector& v);
	void					operator-=(const CScalableVector& v);

	CScalableFloat			Index(int i) const;
	CScalableFloat&			Index(int i);

	CScalableFloat			operator[](int i) const;
	CScalableFloat&			operator[](int i);

	bool					operator==(const CScalableVector& u) const;

	operator Vector() const;

public:
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

	void					SetRotation(const Quaternion& q);

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

	operator Quaternion() const;
	operator Matrix4x4() const;

public:
	float					m[3][3];
	CScalableVector			mt;
};

inline CScalableVector operator*( Vector v, const CScalableFloat& f )
{
	return CScalableVector( f*v.x, f*v.y, f*v.z );
}

bool LineSegmentIntersectsSphere(const CScalableVector& v1, const CScalableVector& v2, const CScalableVector& s, const CScalableFloat& flRadius, CScalableVector& vecPoint);

#endif
