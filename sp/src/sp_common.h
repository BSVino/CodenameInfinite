#ifndef SP_COMMON_H
#define SP_COMMON_H

#include <tvector.h>
#include <vector.h>
#include <common.h>
#include <quaternion.h>

typedef enum
{
	SCALE_RENDER = -1,	// SCALE_RENDER is meters but the view frustum is different
	SCALE_NONE = 0,
	SCALE_MILLIMETER,
	SCALE_METER,
	SCALE_KILOMETER,	// 1000m
	SCALE_MEGAMETER,	// 1000km
	SCALE_GIGAMETER,	// 1000Mm
	SCALE_TERAMETER,	// 1000Gm - A terameter is about 6.685 astronomical units.
	SCALE_HIGHEST = SCALE_TERAMETER,
} scale_t;

// The scale stack skips SCALE_NONE. SCALE_MILLIMETER is 0. 
#define SCALESTACK_SIZE (SCALE_HIGHEST)
#define SCALESTACK_INDEX(x) (x-1)
#define SCALESTACK_SCALE(x) ((scale_t)(x+1))

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
	bool					operator!=(const CScalableFloat& u) const;
	bool					operator<(const CScalableFloat& u) const;
	bool					operator>(const CScalableFloat& u) const;
	bool					operator<(float flMeters) const;
	bool					operator>(float flMeters) const;

	bool					operator<=(const CScalableFloat& u) const;
	bool					operator>=(const CScalableFloat& u) const;

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

inline CScalableFloat RemapValClamped(const CScalableFloat& flInput, const CScalableFloat& flInLo, const CScalableFloat& flInHi, const CScalableFloat& flOutLo, const CScalableFloat& flOutHi)
{
	if (flInput < flInLo)
		return flOutLo;

	if (flInput > flInHi)
		return flOutHi;

	return RemapVal(flInput, flInLo, flInHi, flOutLo, flOutHi);
}

inline double RemapValClamped(const CScalableFloat& flInput, const CScalableFloat& flInLo, const CScalableFloat& flInHi, double flOutLo, double flOutHi)
{
	if (flInput < flInLo)
		return flOutLo;

	if (flInput > flInHi)
		return flOutHi;

	return RemapVal(flInput, flInLo, flInHi, flOutLo, flOutHi);
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

	void                    Normalize();
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

	CScalableVector			operator*( const CScalableVector& v ) const;

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

class DoubleMatrix4x4;

class CScalableMatrix
{
public:
							CScalableMatrix() { Identity(); }
	explicit                CScalableMatrix(const Vector& vecForward, const Vector& vecUp, const Vector& vecRight, const CScalableVector& vecPosition = CScalableVector());
	explicit                CScalableMatrix(const EAngle& angDirection, const CScalableVector& vecPosition=CScalableVector(0,0,0));
	explicit                CScalableMatrix(const Matrix4x4& mOther);
	explicit                CScalableMatrix(const DoubleMatrix4x4& mOther);

public:
	void					Identity();
	bool					IsIdentity();

	void					SetTranslation(const CScalableVector& m) { mt = m; }

	void                    SetAngles(const EAngle& angDir);
	void                    SetRotation(float flAngle, const Vector& vecAxis);		// Assumes the axis is a normalized vector.
	void                    SetRotation(const Quaternion& q);
	void                    SetOrientation(const Vector& vecDir, const Vector& vecUp = Vector(0, 1, 0));

	// Add a transformation
	CScalableMatrix         AddTranslation(const CScalableVector& v);
	CScalableMatrix         AddAngles(const EAngle& a);

	const CScalableVector&  GetTranslation() const { return mt; }
	EAngle                  GetAngles() const;

	// Add a translation
	CScalableMatrix         operator+=(const CScalableVector& v);
	// Add a rotation
	CScalableMatrix         operator+=(const EAngle& a);

	// Add a transformation
	CScalableMatrix         operator*(const CScalableMatrix& t) const;
	CScalableMatrix         operator*=(const CScalableMatrix& t);

	// Transform a vector
	CScalableVector			operator*(const CScalableVector& v) const;

	CScalableVector			TransformVector(const CScalableVector& v) const;

	bool                    operator==(const CScalableMatrix& v) const;
	bool                    operator!=(const CScalableMatrix& v) const;

	void					InvertRT();
	CScalableMatrix			InvertedRT() const;

	void					SetColumn(int i, const Vector& vecColumn);
	Vector					GetColumn(int i) const;

	Vector					GetRow(int i) const;

	void                    SetForwardVector(const Vector& vecForward);
	void                    SetUpVector(const Vector& vecUp);
	void                    SetRightVector(const Vector& vecRight);
	Vector					GetForwardVector() const { return GetRow(0); }
	Vector					GetUpVector() const { return GetRow(1); }
	Vector					GetRightVector() const { return GetRow(2); }

	class DoubleMatrix4x4   GetUnits(scale_t eScale) const;

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

class CScalableCollisionResult
{
public:
	CScalableCollisionResult()
	{
		flFraction = 1;
		bHit = false;
		bStartInside = false;
	}

public:
	bool				bHit;
	bool				bStartInside;
	double				flFraction;
	CScalableVector		vecHit;
	CScalableVector		vecNormal;
};

bool LineSegmentIntersectsSphere(const CScalableVector& v1, const CScalableVector& v2, const CScalableVector& s, const CScalableFloat& flRadius, CScalableCollisionResult& tr);
bool LineSegmentIntersectsTriangle(const CScalableVector& s0, const CScalableVector& s1, const CScalableVector& v0, const CScalableVector& v1, const CScalableVector& v2, CScalableCollisionResult& tr);

#endif
