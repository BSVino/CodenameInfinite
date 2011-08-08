#include "sp_common.h"

#include <matrix.h>
#include <tstring.h>

CScalableFloat::CScalableFloat()
{
	memset(&m_aflScaleStack[0], 0, SCALESTACK_SIZE*sizeof(float));
}

CScalableFloat::CScalableFloat(float flUnits, scale_t eScale)
{
	memset(&m_aflScaleStack[0], 0, SCALESTACK_SIZE*sizeof(float));

	int i = SCALESTACK_INDEX(eScale);
	m_aflScaleStack[i] = flUnits;
	m_bPositive = (flUnits > 0);
}

static float g_aflValueExponents[] = 
{
	1000.0f,
	1000000.0f,
	1000000000.0f,
	1000000000000.0f,
	1000000000000000.0f,
};

static float g_aflPowNeg2Exponents[] = 
{
	1.0f,
	0.5f,
	0.25f,
	0.125f,
	0.0625f,
	0.03125f,
	0.015625f,
	0.0078125f,
	0.00390625f,
	0.001953125f,
	0.0009765625f,
	0.00048828125f,
	0.000244140625f,
	0.0001220703125f,
	6.103515625E-5f,
	3.0517578125E-5f,
	1.52587890625E-5f,
	7.62939453125E-6f,
	3.814697265625E-6f,
	1.9073486328125E-6f,
	9.5367431640625E-7f,
	4.7683715820312E-7f,
	2.3841857910156E-7f,
	1.1920928955078E-7f,
	5.9604644775391E-8f,
	2.9802322387695E-8f,
	1.4901161193848E-8f,
	7.4505805969238E-9f,
	3.7252902984619E-9f,
	1.862645149231E-9f,
	9.3132257461548E-10f,
	4.6566128730774E-10f,
	2.3283064365387E-10f,
	1.1641532182693E-10f,
	5.8207660913467E-11f,
	2.9103830456734E-11f,
	1.4551915228367E-11f,
	7.2759576141834E-12f,
	3.6379788070917E-12f,
	1.8189894035459E-12f,
	9.0949470177293E-13f,
	4.5474735088646E-13f,
	2.2737367544323E-13f,
	1.1368683772162E-13f,
	5.6843418860808E-14f,
	2.8421709430404E-14f,
	1.4210854715202E-14f,
	7.105427357601E-15f,
	3.5527136788005E-15f,
	1.7763568394003E-15f,
	8.8817841970013E-16f,
	4.4408920985006E-16f,
	2.2204460492503E-16f,
};

typedef unsigned long long tlong;
CScalableFloat::CScalableFloat(double flUnits, scale_t eScale)
{
	memset(&m_aflScaleStack[0], 0, SCALESTACK_SIZE*sizeof(float));

	tlong l = *(tlong*)((double*)&flUnits);

	if (flUnits == 0)
		return;

	// We need to preserve the precision of the double, so we need to split it up.
	// If we have a double like 1.234567890123456Gm then we need to make it into
	// 1.234Gm 5.678Mm 9.012Km and 3.456m. The best way to do that is to break into
	// the IEEE double precision floating point standard, parse the data there, and
	// split it up into different scales for our scalable float. Since the floats
	// are stored in binary we won't quite get those perfect 4-digit number splits
	// but in the end it should all add back up to the original number.

	TAssert(sizeof(double) == 8);

#ifdef _DEBUG
	// Test to make sure we're dealing with IEEE 64 bit doubles
	tlong iTest = 0x3FF0000000000000;
	TAssert(*(double*)(&iTest) == 1.0);

	iTest = 0x4000000000000000;
	TAssert(*(double*)(&iTest) == 2.0);

	iTest = 0xc000000000000000;
	TAssert(*(double*)(&iTest) == -2.0);
#endif

	// IEEE double layout is 1 sign bit, 11 exponent bits, and 52 fraction bits. One for each card in the deck.
	// value = -sign * 1.fraction * 2^(exponent-1023)
	// The fraction is in base 2.
	const int iFB = 52;

	bool bSign = !!(l & ((tlong)1<<63));
	int iExponent = ((tlong)(l & ((tlong)0x7ff << iFB)) >> iFB) - 1023;
	tlong iFraction = (tlong)(l & ((tlong)0xfffffffffffff)); // The last 52 bits.

	TAssert( iExponent <= 1023 && iExponent >= -1022 );

	float flExponent;
	if (iExponent > 0)
		flExponent = (float)((tlong)1<<iExponent);
	else if (-iExponent <= sizeof(g_aflPowNeg2Exponents)/sizeof(g_aflPowNeg2Exponents[0]))
		flExponent = g_aflPowNeg2Exponents[-iExponent];
	else
		flExponent = pow(2.0f, iExponent);

	// If exponent is 0 and the value isn't 0 then we have a subnormal float which is not allowed.
	TAssert(((tlong)(l & ((tlong)0x7ff << iFB)) >> iFB) != 0);

	// If exponent is 0x7ff (all 1's) we have infinity or NaN and we don't allow either.
	TAssert(((tlong)(l & ((tlong)0x7ff << iFB)) >> iFB) != 0x7ff);

	TAssert(iFB <= sizeof(g_aflPowNeg2Exponents)/sizeof(g_aflPowNeg2Exponents[0]));

	int iSign = bSign?-1:1;
	int iDigits = 7;
	int iRemaining = iFB - iDigits;

	// Add the 1 for the first part since it's 1.fraction but cut off all other digits that won't fit in a 32 bit floating point.
	long iFirst = (long)((iFraction + ((tlong)1 << iFB)) >> iRemaining) * iSign;
	// value = iFirst * 2^exponent * 2^-(52-remaining)
	float flFirst = ((float)iFirst) * flExponent * g_aflPowNeg2Exponents[iFB-iRemaining];

	int iScale = SCALESTACK_INDEX(eScale);

	while (flFirst < 1 && flFirst > 0 && iScale > 0)
	{
		iScale--;
		flFirst *= 1000;
		flExponent *= 1000;
	}

	while (flFirst > -1 && flFirst < 0 && iScale > 0)
	{
		iScale--;
		flFirst *= 1000;
		flExponent *= 1000;
	}

	m_aflScaleStack[iScale--] = flFirst;

	for (size_t i = 0; i < 4; i++)
	{
		if (iScale < 0)
			break;

		// Take the next few digits.
		if (iScale == 0 || i == 3)
		{
			iDigits = iRemaining;
			if (iDigits >= sizeof(long)*8)
				iDigits = sizeof(long)*8-1;
		}
		else
			iDigits = (i%2)?11:10;

		if (iDigits > iRemaining)
			iDigits = iRemaining;

		tlong iMask = ((tlong)1<<iDigits)-1;
		iRemaining -= iDigits;

		TAssert(iDigits <= sizeof(long)*8);

		long iValue = (long)((iFraction & (iMask << iRemaining)) >> iRemaining) * iSign;
		float flValue = ((float)iValue) * flExponent * g_aflPowNeg2Exponents[iFB-iRemaining];

		m_aflScaleStack[iScale--] = flValue * g_aflValueExponents[i];

		if (iScale < 0)
			break;
		if (iRemaining <= 0)
			break;
	}

	m_bPositive = (flUnits > 0);
}

float CScalableFloat::GetUnits(scale_t eScale) const
{
	float flResult = 0;
	for (size_t i = 0; i < SCALESTACK_SIZE; i++)
		flResult += ConvertUnits(m_aflScaleStack[i], (scale_t)(i+1), eScale);

	return flResult;
}

bool CScalableFloat::IsZero() const
{
	for (size_t i = 0; i < SCALESTACK_SIZE; i++)
	{
		if (m_aflScaleStack[i] != 0)
			return false;
	}

	return true;
}

CScalableFloat CScalableFloat::operator-() const
{
	CScalableFloat f;

	for (size_t i = 0; i < SCALESTACK_SIZE; i++)
		f.m_aflScaleStack[i] = -m_aflScaleStack[i];

	f.m_bPositive = !m_bPositive;

	return f;
}

CScalableFloat CScalableFloat::operator+(const CScalableFloat& u) const
{
	CScalableFloat flReturn;

	for (size_t i = 0; i < SCALESTACK_SIZE; i++)
		flReturn.m_aflScaleStack[i] = m_aflScaleStack[i] + u.m_aflScaleStack[i];

	if (m_bPositive && u.m_bPositive)
		flReturn.m_bPositive = true;
	else if (!m_bPositive && !u.m_bPositive)
		flReturn.m_bPositive = false;
	else
		flReturn.m_bPositive = (flReturn.GetUnits(SCALE_KILOMETER) > 0);

	flReturn.NormalizeScaleStack();

	return flReturn;
}

CScalableFloat CScalableFloat::operator-(const CScalableFloat& u) const
{
	CScalableFloat flReturn;

	for (size_t i = 0; i < SCALESTACK_SIZE; i++)
		flReturn.m_aflScaleStack[i] = m_aflScaleStack[i] - u.m_aflScaleStack[i];

	if (m_bPositive && !u.m_bPositive)
		flReturn.m_bPositive = true;
	else if (!m_bPositive && u.m_bPositive)
		flReturn.m_bPositive = false;
	else
		flReturn.m_bPositive = (flReturn.GetUnits(SCALE_KILOMETER) > 0);

	flReturn.NormalizeScaleStack();

	return flReturn;
}

void CScalableFloat::operator+=(const CScalableFloat& u)
{
	for (size_t i = 0; i < SCALESTACK_SIZE; i++)
		m_aflScaleStack[i] = m_aflScaleStack[i] + u.m_aflScaleStack[i];

	if (m_bPositive && u.m_bPositive)
		m_bPositive = true;
	else if (!m_bPositive && !u.m_bPositive)
		m_bPositive = false;
	else
		m_bPositive = (GetUnits(SCALE_KILOMETER) > 0);

	NormalizeScaleStack();
}

void CScalableFloat::operator-=(const CScalableFloat& u)
{
	for (size_t i = 0; i < SCALESTACK_SIZE; i++)
		m_aflScaleStack[i] = m_aflScaleStack[i] - u.m_aflScaleStack[i];

	if (m_bPositive && !u.m_bPositive)
		m_bPositive = true;
	else if (!m_bPositive && u.m_bPositive)
		m_bPositive = false;
	else
		m_bPositive = (GetUnits(SCALE_KILOMETER) > 0);

	NormalizeScaleStack();
}

CScalableFloat CScalableFloat::operator*( const CScalableFloat& f ) const
{
	// Since I hard-coded the code below, this would have to be updated if I add a scale.
	TAssert(SCALESTACK_SIZE == SCALE_TERAMETER);

	CScalableFloat flReturn;

	flReturn.m_aflScaleStack[0] = m_aflScaleStack[0] * f.m_aflScaleStack[0];
	flReturn.m_aflScaleStack[1] = m_aflScaleStack[1] * f.m_aflScaleStack[0] + m_aflScaleStack[0] * f.m_aflScaleStack[1];
	flReturn.m_aflScaleStack[2] = m_aflScaleStack[2] * f.m_aflScaleStack[0] + m_aflScaleStack[1] * f.m_aflScaleStack[1] + m_aflScaleStack[0] * f.m_aflScaleStack[2];
	flReturn.m_aflScaleStack[3] = m_aflScaleStack[3] * f.m_aflScaleStack[0] + m_aflScaleStack[2] * f.m_aflScaleStack[1]
		+ m_aflScaleStack[1] * f.m_aflScaleStack[2] + m_aflScaleStack[0] * f.m_aflScaleStack[3];
	flReturn.m_aflScaleStack[4] = m_aflScaleStack[4] * f.m_aflScaleStack[0] + m_aflScaleStack[3] * f.m_aflScaleStack[1]
		+ m_aflScaleStack[2] * f.m_aflScaleStack[2] + m_aflScaleStack[1] * f.m_aflScaleStack[3] + m_aflScaleStack[0] * f.m_aflScaleStack[4];
	float fl5 = m_aflScaleStack[4] * f.m_aflScaleStack[1] + m_aflScaleStack[3] * f.m_aflScaleStack[2]
		+ m_aflScaleStack[2] * f.m_aflScaleStack[3] + m_aflScaleStack[1] * f.m_aflScaleStack[4];
	float fl6 = m_aflScaleStack[4] * f.m_aflScaleStack[2] + m_aflScaleStack[3] * f.m_aflScaleStack[3] + m_aflScaleStack[2] * f.m_aflScaleStack[4];
	float fl7 = m_aflScaleStack[4] * f.m_aflScaleStack[3] + m_aflScaleStack[3] * f.m_aflScaleStack[4];
	float fl8 = m_aflScaleStack[4] * f.m_aflScaleStack[4];

	flReturn.m_aflScaleStack[4] += fl5*1000;
	flReturn.m_aflScaleStack[4] += fl6*1000000;
	flReturn.m_aflScaleStack[4] += fl7*1000000000;
	flReturn.m_aflScaleStack[4] += fl8*1000000000000;

	flReturn.m_bPositive = f.m_bPositive?m_bPositive:!m_bPositive;
	flReturn.NormalizeScaleStack();

	return flReturn;
}

CScalableFloat CScalableFloat::operator/( const CScalableFloat& f ) const
{
	// I don't like doing this but there's really no way to do a proper division.
	float flDivide = f.GetUnits(SCALE_METER);

	TAssert(flDivide != 0);
	if (flDivide == 0)
		return CScalableFloat();

	CScalableFloat flReturn;

	flReturn.m_bPositive = f.m_bPositive?m_bPositive:!m_bPositive;

	for (size_t i = 0; i < SCALESTACK_SIZE; i++)
	{
		if (m_aflScaleStack[i] == 0)
			continue;

		float flResult = m_aflScaleStack[i]/flDivide;
		int j = i;

		if (flReturn.m_bPositive)
		{
			while (flResult > 0 && flResult < 0.001f && j >= 0)
			{
				flResult *= 1000;
				j--;
			}
		}
		else
		{
			while (flResult < 0 && flResult > -0.001f && j >= 0)
			{
				flResult *= 1000;
				j--;
			}
		}

		flReturn.m_aflScaleStack[j] += flResult;
	}

	flReturn.NormalizeScaleStack();

	return flReturn;
}

CScalableFloat CScalableFloat::operator*(float f) const
{
	CScalableFloat flReturn;

	for (size_t i = 0; i < SCALESTACK_SIZE; i++)
		flReturn.m_aflScaleStack[i] = m_aflScaleStack[i] * f;

	flReturn.m_bPositive = (f > 0)?m_bPositive:!m_bPositive;
	flReturn.NormalizeScaleStack();

	return flReturn;
}

CScalableFloat CScalableFloat::operator/(float f) const
{
	CScalableFloat flReturn;

	for (size_t i = 0; i < SCALESTACK_SIZE; i++)
		flReturn.m_aflScaleStack[i] = m_aflScaleStack[i] / f;

	flReturn.m_bPositive = (f > 0)?m_bPositive:!m_bPositive;
	flReturn.NormalizeScaleStack();

	return flReturn;
}

CScalableFloat CScalableFloat::AddMultiple(const CScalableFloat& f, const CScalableFloat& g) const
{
	CScalableFloat flReturn;

	for (size_t i = 0; i < SCALESTACK_SIZE; i++)
		flReturn.m_aflScaleStack[i] = m_aflScaleStack[i] + f.m_aflScaleStack[i] + g.m_aflScaleStack[i];

	if (m_bPositive && f.m_bPositive && g.m_bPositive)
		flReturn.m_bPositive = true;
	else if (!m_bPositive && !f.m_bPositive && !g.m_bPositive)
		flReturn.m_bPositive = false;
	else
		flReturn.m_bPositive = (flReturn.GetUnits(SCALE_KILOMETER) > 0);

	flReturn.NormalizeScaleStack();

	return flReturn;
}

CScalableFloat CScalableFloat::AddMultiple(const CScalableFloat& f, const CScalableFloat& g, const CScalableFloat& h) const
{
	CScalableFloat flReturn;

	for (size_t i = 0; i < SCALESTACK_SIZE; i++)
		flReturn.m_aflScaleStack[i] = m_aflScaleStack[i] + f.m_aflScaleStack[i] + g.m_aflScaleStack[i] + h.m_aflScaleStack[i];

	if (m_bPositive && f.m_bPositive && g.m_bPositive && h.m_bPositive)
		flReturn.m_bPositive = true;
	else if (!m_bPositive && !f.m_bPositive && !g.m_bPositive && !h.m_bPositive)
		flReturn.m_bPositive = false;
	else
		flReturn.m_bPositive = (flReturn.GetUnits(SCALE_KILOMETER) > 0);

	flReturn.NormalizeScaleStack();

	return flReturn;
}

void CScalableFloat::NormalizeScaleStack()
{
	for (size_t i = 0; i < SCALESTACK_SIZE; i++)
	{
		if (m_aflScaleStack[i] == 0)
			continue;

		if (m_bPositive)
		{
			if (i < SCALESTACK_SIZE-1)
			{
				if (m_aflScaleStack[i] > 0 && m_aflScaleStack[i+1] < 0 && m_aflScaleStack[i+1] > -1)
				{
					m_aflScaleStack[i] += m_aflScaleStack[i+1]*1000;
					m_aflScaleStack[i+1] = 0;
				}

				if (m_aflScaleStack[i] < 0 && m_aflScaleStack[i+1] > 0 && m_aflScaleStack[i+1] < 1)
				{
					m_aflScaleStack[i] += m_aflScaleStack[i+1]*1000;
					m_aflScaleStack[i+1] = 0;
				}

				while (m_aflScaleStack[i] < -0.2f && m_aflScaleStack[i+1] > 1)
				{
					m_aflScaleStack[i] += 1000;
					m_aflScaleStack[i+1] -= 1;
				}

				while (m_aflScaleStack[i] > 1200)
				{
					m_aflScaleStack[i] -= 1000;
					m_aflScaleStack[i+1] += 1;
				}
			}

			if (i > 0)
			{
				if (m_aflScaleStack[i] > 0 && m_aflScaleStack[i] < 0.001f)
				{
					m_aflScaleStack[i-1] += m_aflScaleStack[i]*1000;
					m_aflScaleStack[i] = 0;
				}
			}
		}
		else
		{
			if (i < SCALESTACK_SIZE-1)
			{
				if (m_aflScaleStack[i] < 0 && m_aflScaleStack[i+1] > 0 && m_aflScaleStack[i+1] < 1)
				{
					m_aflScaleStack[i] += m_aflScaleStack[i+1]*1000;
					m_aflScaleStack[i+1] = 0;
				}

				if (m_aflScaleStack[i] > 0 && m_aflScaleStack[i+1] < 0 && m_aflScaleStack[i+1] > -1)
				{
					m_aflScaleStack[i] += m_aflScaleStack[i+1]*1000;
					m_aflScaleStack[i+1] = 0;
				}

				while (m_aflScaleStack[i] > 0.2f && m_aflScaleStack[i+1] < -1)
				{
					m_aflScaleStack[i] -= 1000;
					m_aflScaleStack[i+1] += 1;
				}

				while (m_aflScaleStack[i] < -1200)
				{
					m_aflScaleStack[i] += 1000;
					m_aflScaleStack[i+1] -= 1;
				}
			}

			if (i > 0)
			{
				if (m_aflScaleStack[i] < 0 && m_aflScaleStack[i] > -0.001f)
				{
					m_aflScaleStack[i-1] += m_aflScaleStack[i]*1000;
					m_aflScaleStack[i] = 0;
				}
			}
		}
	}
}

bool CScalableFloat::operator<(const CScalableFloat& u) const
{
	if (m_bPositive && !u.m_bPositive)
		return false;

	if (!m_bPositive && u.m_bPositive)
		return true;

	CScalableFloat flDifference = u - *this;
	return flDifference.m_bPositive;
}

bool CScalableFloat::operator>(const CScalableFloat& u) const
{
	if (m_bPositive && !u.m_bPositive)
		return true;

	if (!m_bPositive && u.m_bPositive)
		return false;

	CScalableFloat flDifference = *this - u;
	return flDifference.m_bPositive;
}

static float g_aflConversions[9] = 
{
	0.000000000001f,
	0.000000001f,
	0.000001f,
	0.001f,
	1.0f,
	1000.0f,
	1000000.0f,
	1000000000.0f,
	1000000000000.0f,
};

float CScalableFloat::ConvertUnits(float flUnits, scale_t eFrom, scale_t eTo)
{
	if (eFrom == eTo)
		return flUnits;

	if (flUnits == 0)
		return 0;

	TAssert( eTo >= SCALE_METER && eTo <= SCALE_HIGHEST );
	if (eTo == SCALE_NONE)
		return 0;

	if (eTo < SCALE_NONE)
		return 0;

	if (eTo > SCALE_HIGHEST)
		return 0;

	TAssert( SCALE_HIGHEST+4 <= 9 );
	return flUnits * g_aflConversions[eFrom-eTo+4];
}

CScalableVector::CScalableVector(const CScalableFloat& X, const CScalableFloat& Y, const CScalableFloat& Z)
{
	x = X;
	y = Y;
	z = Z;
}

CScalableVector::CScalableVector(Vector vecUnits, scale_t eScale)
{
	x = CScalableFloat(vecUnits.x, eScale);
	y = CScalableFloat(vecUnits.y, eScale);
	z = CScalableFloat(vecUnits.z, eScale);
}

CScalableVector::CScalableVector(DoubleVector vecUnits, scale_t eScale)
{
	x = CScalableFloat(vecUnits.x, eScale);
	y = CScalableFloat(vecUnits.y, eScale);
	z = CScalableFloat(vecUnits.z, eScale);
}

Vector CScalableVector::GetUnits(scale_t eScale) const
{
	Vector vecResult = Vector(0,0,0);

	vecResult.x = x.GetUnits(eScale);
	vecResult.y = y.GetUnits(eScale);
	vecResult.z = z.GetUnits(eScale);

	return vecResult;
}

bool CScalableVector::IsZero() const
{
	return x.IsZero() && y.IsZero() && z.IsZero();
}

CScalableFloat CScalableVector::Length() const
{
	CScalableFloat f = x*x + y*y + z*z;
	CScalableFloat r(sqrt(f.GetUnits(SCALE_METER)), SCALE_METER);

	// Instead of calling ::NormalizeScaleStack() do this instead. It's faster because it doesn't normalize each level in groups of 1000.
	TAssert(SCALESTACK_SIZE == SCALE_TERAMETER);

	while (r.m_aflScaleStack[0] > 1200000000000)
	{
		r.m_aflScaleStack[0] -= 1000000000000;
		r.m_aflScaleStack[4] += 1;
	}

	while (r.m_aflScaleStack[0] > 1200000000)
	{
		r.m_aflScaleStack[0] -= 1000000000;
		r.m_aflScaleStack[3] += 1;
	}

	while (r.m_aflScaleStack[0] > 1200000)
	{
		r.m_aflScaleStack[0] -= 1000000;
		r.m_aflScaleStack[2] += 1;
	}

	while (r.m_aflScaleStack[0] > 120000)
	{
		r.m_aflScaleStack[0] -= 100000;
		r.m_aflScaleStack[1] += 100;
	}

	while (r.m_aflScaleStack[0] > 12000)
	{
		r.m_aflScaleStack[0] -= 10000;
		r.m_aflScaleStack[1] += 10;
	}

	while (r.m_aflScaleStack[0] > 1200)
	{
		r.m_aflScaleStack[0] -= 1000;
		r.m_aflScaleStack[1] += 1;
	}

	return r;
}

CScalableFloat CScalableVector::LengthSqr() const
{
	return x*x + y*y + z*z;
}

CScalableVector CScalableVector::operator-() const
{
	CScalableVector v;
	v.x = -x;
	v.y = -y;
	v.z = -z;
	return v;
}

CScalableVector CScalableVector::operator+( const CScalableVector& v ) const
{
	CScalableVector vecReturn;

	vecReturn.x = x + v.x;
	vecReturn.y = y + v.y;
	vecReturn.z = z + v.z;

	return vecReturn;
}

CScalableVector CScalableVector::operator-( const CScalableVector& v ) const
{
	CScalableVector vecReturn;

	vecReturn.x = x - v.x;
	vecReturn.y = y - v.y;
	vecReturn.z = z - v.z;

	return vecReturn;
}

CScalableVector CScalableVector::operator*( const CScalableFloat& f ) const
{
	CScalableVector vecReturn;
	for (size_t i = 0; i < 3; i++)
		vecReturn[i] = Index(i) * f;

	return vecReturn;
}

CScalableVector CScalableVector::operator/( const CScalableFloat& f ) const
{
	CScalableVector vecReturn;
	for (size_t i = 0; i < 3; i++)
		vecReturn[i] = Index(i) / f;

	return vecReturn;
}

CScalableVector CScalableVector::operator*( float f ) const
{
	CScalableVector vecReturn;
	for (size_t i = 0; i < 3; i++)
		vecReturn[i] = Index(i) * f;

	return vecReturn;
}

CScalableVector CScalableVector::operator/( float f ) const
{
	CScalableVector vecReturn;
	for (size_t i = 0; i < 3; i++)
		vecReturn[i] = Index(i) / f;

	return vecReturn;
}

CScalableFloat CScalableVector::Index(int i) const
{
	if (i == 0)
		return x;
	if (i == 1)
		return y;
	if (i == 2)
		return z;

	TAssert(false);
	return x;
}

CScalableFloat& CScalableVector::Index(int i)
{
	if (i == 0)
		return x;
	if (i == 1)
		return y;
	if (i == 2)
		return z;

	TAssert(false);
	return x;
}

CScalableFloat CScalableVector::operator[](int i) const
{
	return Index(i);
}

CScalableFloat& CScalableVector::operator[](int i)
{
	return Index(i);
}

CScalableMatrix::CScalableMatrix(const Vector& vecForward, const Vector& vecUp, const Vector& vecRight, const CScalableVector& vecPosition)
{
	SetColumn(0, vecForward);
	SetColumn(1, vecUp);
	SetColumn(2, vecRight);
	SetTranslation(vecPosition);
}

void CScalableMatrix::Identity()
{
	memset(this, 0, sizeof(m));

	m[0][0] = m[1][1] = m[2][2] = 1;

	SetTranslation(CScalableVector());
}

void CScalableMatrix::SetAngles(const EAngle& angDir)
{
	float sp = sin(angDir.p * M_PI/180);
	float sy = sin(angDir.y * M_PI/180);
	float sr = sin(angDir.r * M_PI/180);
	float cp = cos(angDir.p * M_PI/180);
	float cy = cos(angDir.y * M_PI/180);
	float cr = cos(angDir.r * M_PI/180);

	m[0][0] = cy*cp;
	m[0][1] = sr*sy-sp*cr*cy;
	m[0][2] = sp*sr*cy+cr*sy;
	m[1][0] = sp;
	m[1][1] = cp*cr;
	m[1][2] = -cp*sr;
	m[2][0] = -sy*cp;
	m[2][1] = sp*cr*sy+sr*cy;
	m[2][2] = cr*cy-sy*sp*sr;
}

EAngle CScalableMatrix::GetAngles() const
{
	if (m[1][0] > 0.99f)
		return EAngle(90, atan2(m[0][2], m[2][2]) * 180/M_PI, 0);
	else if (m[1][0] < -0.99f)
		return EAngle(-90, atan2(m[0][2], m[2][2]) * 180/M_PI, 0);

	// Clamp to [-1, 1] looping
	float flPitch = fmod(m[1][0], 2);
	if (flPitch > 1)
		flPitch -= 2;
	else if (flPitch < -1)
		flPitch += 2;

	return EAngle(asin(flPitch) * 180/M_PI, atan2(-m[2][0], m[0][0]) * 180/M_PI, atan2(-m[1][2], m[1][1]) * 180/M_PI);
}

CScalableMatrix CScalableMatrix::operator*(const CScalableMatrix& t) const
{
	CScalableMatrix r;

	r.m[0][0] = m[0][0]*t.m[0][0] + m[0][1]*t.m[1][0] + m[0][2]*t.m[2][0];
	r.m[0][1] = m[0][0]*t.m[0][1] + m[0][1]*t.m[1][1] + m[0][2]*t.m[2][1];
	r.m[0][2] = m[0][0]*t.m[0][2] + m[0][1]*t.m[1][2] + m[0][2]*t.m[2][2];

	r.m[1][0] = m[1][0]*t.m[0][0] + m[1][1]*t.m[1][0] + m[1][2]*t.m[2][0];
	r.m[1][1] = m[1][0]*t.m[0][1] + m[1][1]*t.m[1][1] + m[1][2]*t.m[2][1];
	r.m[1][2] = m[1][0]*t.m[0][2] + m[1][1]*t.m[1][2] + m[1][2]*t.m[2][2];

	r.m[2][0] = m[2][0]*t.m[0][0] + m[2][1]*t.m[1][0] + m[2][2]*t.m[2][0];
	r.m[2][1] = m[2][0]*t.m[0][1] + m[2][1]*t.m[1][1] + m[2][2]*t.m[2][1];
	r.m[2][2] = m[2][0]*t.m[0][2] + m[2][1]*t.m[1][2] + m[2][2]*t.m[2][2];

	r.mt[0] = t.mt[0]*m[0][0] + t.mt[1]*m[0][1] + t.mt[2]*m[0][2] + mt[0];
	r.mt[1] = t.mt[0]*m[1][0] + t.mt[1]*m[1][1] + t.mt[2]*m[1][2] + mt[1];
	r.mt[2] = t.mt[0]*m[2][0] + t.mt[1]*m[2][1] + t.mt[2]*m[2][2] + mt[2];

	return r;
}

CScalableVector CScalableMatrix::operator*(const CScalableVector& v) const
{
	CScalableVector vecResult;
	vecResult.x = (v.x * m[0][0]).AddMultiple(v.y * m[0][1], v.z * m[0][2], mt[0]);
	vecResult.y = (v.x * m[1][0]).AddMultiple(v.y * m[1][1], v.z * m[1][2], mt[1]);
	vecResult.z = (v.x * m[2][0]).AddMultiple(v.y * m[2][1], v.z * m[2][2], mt[2]);

	return vecResult;
}

CScalableVector CScalableMatrix::TransformNoTranslate(const CScalableVector& v) const
{
	CScalableVector vecResult;
	vecResult.x = (v.x * m[0][0]).AddMultiple(v.y * m[0][1], v.z * m[0][2]);
	vecResult.y = (v.x * m[1][0]).AddMultiple(v.y * m[1][1], v.z * m[1][2]);
	vecResult.z = (v.x * m[2][0]).AddMultiple(v.y * m[2][1], v.z * m[2][2]);

	return vecResult;
}

// Not a true inversion, only works if the matrix is a translation/rotation matrix.
void CScalableMatrix::InvertTR()
{
	CScalableMatrix t;

	for (int h = 0; h < 3; h++)
		for (int v = 0; v < 3; v++)
			t.m[h][v] = m[v][h];

	CScalableFloat fl03 = mt[0];
	CScalableFloat fl13 = mt[1];
	CScalableFloat fl23 = mt[2];

	(*this) = t;

	SetTranslation(t*CScalableVector(-fl03, -fl13, -fl23));
}

void CScalableMatrix::SetColumn(int i, const Vector& vecColumn)
{
	m[0][i] = vecColumn.x;
	m[1][i] = vecColumn.y;
	m[2][i] = vecColumn.z;
}

Vector CScalableMatrix::GetColumn(int i) const
{
	return Vector(m[0][i], m[1][i], m[2][i]);
}

Matrix4x4 CScalableMatrix::GetUnits(scale_t eScale) const
{
	Matrix4x4 r;
	r.Init(m[0][0], m[0][1], m[0][2], 0, m[1][0], m[1][1], m[1][2], 0, m[2][0], m[2][1], m[2][2], 0, 0, 0, 0, 1);
	r.SetTranslation(mt.GetUnits(eScale));
	return r;
}
