#ifndef LW_MATRIX_H
#define LW_MATRIX_H

#include <vector.h>

// The red pill

class Quaternion;

class Matrix4x4
{
public:
				Matrix4x4() { Identity(); }
				Matrix4x4(float m00, float m01, float m02, float m03, float m10, float m11, float m12, float m13, float m20, float m21, float m22, float m23, float m30, float m31, float m32, float m33);
				Matrix4x4(const Matrix4x4& m);
				Matrix4x4(float* aflValues);
				Matrix4x4(const Vector& vecForward, const Vector& vecUp, const Vector& vecRight, const Vector& vecPosition = Vector(0,0,0));
				Matrix4x4(const Quaternion& q);

public:
	void		Identity();
	void		Init(const Matrix4x4& m);
	void		Init(float m00, float m01, float m02, float m03, float m10, float m11, float m12, float m13, float m20, float m21, float m22, float m23, float m30, float m31, float m32, float m33);

	bool		IsIdentity() const;

	Matrix4x4	Transposed() const;

	// Simple matrix operations
	Matrix4x4	operator*(float f) const;
	Matrix4x4	operator+(const Matrix4x4& m) const;

	// Set a transformation
	void		SetTranslation(const Vector& vecPos);
	void		SetAngles(const EAngle& angDir);
	void		SetRotation(float flAngle, const Vector& vecAxis);
	void		SetRotation(const Quaternion& q);
	void		SetOrientation(const Vector& vecDir);
	void		SetScale(const Vector& vecScale);
	void		SetReflection(const Vector& vecPlane);

	// Add a translation
	Matrix4x4	operator+=(const Vector& v);
	// Add a rotation
	Matrix4x4	operator+=(const EAngle& a);

	// Add a transformation
	Matrix4x4	operator*(const Matrix4x4& t) const;
	Matrix4x4	operator*=(const Matrix4x4& t);

	// Add a transformation
	Matrix4x4	AddScale(const Vector& vecScale);

	Vector		GetTranslation() const;
	EAngle		GetAngles() const;

	// Transform a vector
	Vector		operator*(const Vector& v) const;

	Vector4D	GetRow(int i);
	Vector4D	GetColumn(int i) const;
	void		SetColumn(int i, const Vector4D& vecColumn);
	void		SetColumn(int i, const Vector& vecColumn);
	Vector		GetForwardVector() const { return Vector(GetColumn(0)); }
	Vector		GetUpVector() const { return Vector(GetColumn(1)); }
	Vector		GetRightVector() const { return Vector(GetColumn(2)); }

	void		InvertTR();

	float		Trace() const;

	operator float*()
	{
		return(&m[0][0]);
	}

	float		m[4][4];
};

#endif
