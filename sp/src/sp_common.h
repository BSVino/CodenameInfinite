#ifndef SP_COMMON_H
#define SP_COMMON_H

#include <vector.h>

typedef enum
{
	SCALE_NONE = 0,
	SCALE_CENTIMETER,
	SCALE_METER,		// 100cm
	SCALE_KILOMETER,	// 1000m
	SCALE_MEGAMETER,	// 1000km
	SCALE_GIGAMETER,	// 1000Mm
	SCALE_AU,			// 150Gm - Astronomical unit, the distance from the Earth to the Sun
	SCALE_HIGHEST = SCALE_AU,
} scale_t;

template <class T>
class CScalableUnit
{
public:
						CScalableUnit();
						CScalableUnit(T flUnits, scale_t eScale);

public:
	T					GetUnits(scale_t eScale = SCALE_NONE) const;

	CScalableUnit<T>	operator+(const CScalableUnit<T>& u) const;
	CScalableUnit<T>	operator-(const CScalableUnit<T>& u) const;
	CScalableUnit<T>	operator*(const CScalableUnit<T>& u) const;
	CScalableUnit<T>	operator/(const CScalableUnit<T>& u) const;

	CScalableUnit<T>	ConvertUnits(scale_t eScale);

	scale_t				GetScale() const { return m_eScale; }

	// For vectors
	CScalableUnit<T>	Normalized() { return CScalableUnit<T>(m_flUnits.Normalized(), m_eScale); }
	CScalableUnit<float> Length() { return CScalableUnit<float>(m_flUnits.Length(), m_eScale); }

	static T			ConvertUnits(T flUnit, scale_t eFrom, scale_t eTo);

protected:
	T					m_flUnits;
	scale_t				m_eScale;

	static float		s_flScaleConversions[];
};

template <class T>
float CScalableUnit<T>::s_flScaleConversions[] =
{
	0,
	1,		// centimeters
	100,	// cm in a meter
	1000,	// m in a kilometer
	1000,	// km in a megameter
	1000,	// Mm in a kilometer
	150,	// Gm in a lightyear
};

template <class T>
inline CScalableUnit<T>::CScalableUnit()
{
	m_flUnits = 0;
	m_eScale = SCALE_METER;
}

template <class T>
inline CScalableUnit<T>::CScalableUnit(T flUnits, scale_t eScale)
{
	m_flUnits = flUnits;
	m_eScale = eScale;
}

template <class T>
inline T CScalableUnit<T>::GetUnits(scale_t eScale) const
{
	if (eScale == m_eScale)
		return m_flUnits;

	if (eScale == SCALE_NONE)
		return m_flUnits;

	if (eScale < SCALE_NONE)
		return T();

	if (eScale > SCALE_HIGHEST)
		return T();

	if (eScale < m_eScale)
	{
		T flUnits = m_flUnits;
		while (eScale < m_eScale)
		{
			flUnits *= s_flScaleConversions[eScale+1];
			eScale = (scale_t)(eScale+1);
		}
		return flUnits;
	}
	else
	{
		T flUnits = m_flUnits;
		while (eScale > m_eScale)
		{
			flUnits /= s_flScaleConversions[eScale];
			eScale = (scale_t)(eScale-1);
		}
		return flUnits;
	}
}

template <class T>
inline CScalableUnit<T> CScalableUnit<T>::operator+(const CScalableUnit<T>& u) const
{
	if (u.m_eScale == m_eScale)
		return CScalableUnit<T>(u.m_flUnits + m_flUnits, m_eScale);

	if (u.m_eScale > m_eScale)
		return CScalableUnit<T>(GetUnits(u.m_eScale) + u.m_flUnits, u.m_eScale);
	else
		return CScalableUnit<T>(m_flUnits + u.GetUnits(m_eScale), m_eScale);
}

template <class T>
inline CScalableUnit<T> CScalableUnit<T>::operator-(const CScalableUnit<T>& u) const
{
	if (u.m_eScale == m_eScale)
		return CScalableUnit<T>(u.m_flUnits - m_flUnits, m_eScale);

	if (u.m_eScale > m_eScale)
		return CScalableUnit<T>(GetUnits(u.m_eScale) - u.m_flUnits, u.m_eScale);
	else
		return CScalableUnit<T>(m_flUnits - u.GetUnits(m_eScale), m_eScale);
}

template <class T>
inline CScalableUnit<T> CScalableUnit<T>::operator*(const CScalableUnit<T>& u) const
{
	if (u.m_eScale == m_eScale)
		return CScalableUnit<T>(u.m_flUnits * m_flUnits, m_eScale);

	if (u.m_eScale > m_eScale)
		return CScalableUnit<T>(GetUnits(u.m_eScale) * u.m_flUnits, u.m_eScale);
	else
		return CScalableUnit<T>(m_flUnits * u.GetUnits(m_eScale), m_eScale);
}

template <class T>
inline CScalableUnit<T> CScalableUnit<T>::operator/(const CScalableUnit<T>& u) const
{
	if (u.m_eScale == m_eScale)
		return CScalableUnit<T>(u.m_flUnits / m_flUnits, m_eScale);

	if (u.m_eScale > m_eScale)
		return CScalableUnit<T>(GetUnits(u.m_eScale) / u.m_flUnits, u.m_eScale);
	else
		return CScalableUnit<T>(m_flUnits / u.GetUnits(m_eScale), m_eScale);
}

template <class T>
inline CScalableUnit<T> CScalableUnit<T>::ConvertUnits(scale_t eScale)
{
	return CScalableUnit<T>(GetUnits(eScale), eScale);
}

template <class T>
inline T CScalableUnit<T>::ConvertUnits(T flUnits, scale_t eFrom, scale_t eTo)
{
	return CScalableUnit<T>(flUnits, eFrom).GetUnits(eTo);
}

typedef CScalableUnit<float> CScalableFloat;
typedef CScalableUnit<Vector> CScalableVector;

inline CScalableVector operator*( const CScalableVector& v, const CScalableFloat& f )
{
	if (v.GetScale() == f.GetScale())
	{
		Vector vec = v.GetUnits();
		float fl = f.GetUnits();
		return CScalableVector(vec*fl, v.GetScale());
	}

	return CScalableVector(v.GetUnits(v.GetScale())*f.GetUnits(f.GetScale()), f.GetScale());
}

inline CScalableVector operator/( const CScalableVector& v, const CScalableFloat& f )
{
	if (v.GetScale() == f.GetScale())
	{
		Vector vec = v.GetUnits();
		float fl = f.GetUnits();
		return CScalableVector(vec/fl, v.GetScale());
	}

	return CScalableVector(v.GetUnits(v.GetScale())/f.GetUnits(f.GetScale()), f.GetScale());
}

#endif
