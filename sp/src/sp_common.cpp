#include "sp_common.h"

#include <matrix.h>
#include <tstring.h>

#ifdef _DEBUG
#define SANITY_CHECK 1
#endif

#if SANITY_CHECK == 1
#define CHECKSANITY(f) (f).CheckSanity();
#else
#define CHECKSANITY(f)
#endif

CScalableFloat::CScalableFloat()
{
	memset(&m_aiScaleStack[0], 0, SCALESTACK_SIZE*sizeof(short));
	m_bZero = true;
}

CScalableFloat::CScalableFloat(float flUnits, scale_t eScale)
{
	memset(&m_aiScaleStack[0], 0, SCALESTACK_SIZE*sizeof(short));

	if (flUnits == 0)
	{
		m_bZero = true;
		return;
	}

	m_bZero = false;

	int i = SCALESTACK_INDEX(eScale);
	m_bPositive = (flUnits > 0);

	if (m_bPositive)
	{
		while (flUnits < 1)
		{
			flUnits *= 1000;
			i--;
		}

		while (flUnits > 1000)
		{
			flUnits /= 1000;
			i++;
		}

		while (flUnits > 0 && i >= 0)
		{
			m_aiScaleStack[i] = (short)flUnits;
			flUnits -= m_aiScaleStack[i];
			flUnits *= 1000;
			i--;
		}
	}
	else
	{
		while (flUnits > -1)
		{
			flUnits *= 1000;
			i--;
		}

		while (flUnits < -1000)
		{
			flUnits /= 1000;
			i++;
		}

		while (flUnits < 0 && i >= 0)
		{
			m_aiScaleStack[i] = (short)flUnits;
			flUnits -= m_aiScaleStack[i];
			flUnits *= 1000;
			i--;
		}
	}

	CHECKSANITY(*this);
}

CScalableFloat::CScalableFloat(double flUnits, scale_t eScale)
{
	memset(&m_aiScaleStack[0], 0, SCALESTACK_SIZE*sizeof(short));

	if (flUnits == 0)
	{
		m_bZero = true;
		return;
	}

	m_bZero = false;

	int i = SCALESTACK_INDEX(eScale);
	m_bPositive = (flUnits > 0);

	if (m_bPositive)
	{
		while (flUnits < 1)
		{
			flUnits *= 1000;
			i--;
		}

		while (flUnits > 1000)
		{
			flUnits /= 1000;
			i++;
		}

		while (flUnits > 0 && i >= 0)
		{
			m_aiScaleStack[i] = (short)flUnits;
			flUnits -= m_aiScaleStack[i];
			flUnits *= 1000;
			i--;
		}
	}
	else
	{
		while (flUnits > -1)
		{
			flUnits *= 1000;
			i--;
		}

		while (flUnits < -1000)
		{
			flUnits /= 1000;
			i++;
		}

		while (flUnits < 0 && i >= 0)
		{
			m_aiScaleStack[i] = (short)flUnits;
			flUnits -= m_aiScaleStack[i];
			flUnits *= 1000;
			i--;
		}
	}

	CHECKSANITY(*this);
}

double CScalableFloat::GetUnits(scale_t eScale) const
{
	if (m_bZero)
		return 0;

	double flResult = 0;
	for (size_t i = 0; i < SCALESTACK_SIZE; i++)
		flResult += ConvertUnits(m_aiScaleStack[i], (scale_t)(i+1), eScale);

	return flResult;
}

bool CScalableFloat::IsZero() const
{
	if (m_bZero)
		return true;

	for (size_t i = 0; i < SCALESTACK_SIZE; i++)
	{
		if (m_aiScaleStack[i] != 0)
			return false;
	}

	return true;
}

bool CScalableFloat::IsZero()
{
	if (m_bZero)
		return true;

	for (size_t i = 0; i < SCALESTACK_SIZE; i++)
	{
		if (m_aiScaleStack[i] != 0)
			return false;
	}

	m_bZero = true;
	return true;
}

CScalableFloat CScalableFloat::operator-() const
{
	CScalableFloat f;

	for (size_t i = 0; i < SCALESTACK_SIZE; i++)
		f.m_aiScaleStack[i] = -m_aiScaleStack[i];

	f.m_bPositive = !m_bPositive;
	f.m_bZero = m_bZero;

	CHECKSANITY(f);

	return f;
}

CScalableFloat CScalableFloat::AddSimilar(const CScalableFloat& u) const
{
	TAssert(m_bPositive == u.m_bPositive);

	CScalableFloat flReturn;
	flReturn.m_bPositive = m_bPositive;
	for (size_t i = 0; i < SCALESTACK_SIZE; i++)
	{
		flReturn.m_aiScaleStack[i] += m_aiScaleStack[i] + u.m_aiScaleStack[i];
		flReturn.NormalizeStackPosition(i);
	}

	flReturn.m_bZero = m_bZero && u.m_bZero;

	CHECKSANITY(flReturn);

	return flReturn;
}

CScalableFloat CScalableFloat::AddDifferent(const CScalableFloat& u) const
{
	TAssert(m_bPositive != u.m_bPositive);

	CScalableFloat flReturn;
	flReturn.m_bPositive = m_bPositive;

	bool bHighest = true;
	for (int i = SCALESTACK_SIZE-1; i >= 0; i--)
	{
		short iResult = m_aiScaleStack[i] + u.m_aiScaleStack[i];
		if (iResult)
		{
			if (bHighest)
			{
				flReturn.m_bPositive = iResult > 0;
				flReturn.m_aiScaleStack[i] = iResult;
				bHighest = false;
			}
			else
			{
				flReturn.m_aiScaleStack[i] = iResult;

				if (flReturn.m_bPositive)
				{
					int j = i;
					while (flReturn.m_aiScaleStack[j] < 0)
					{
						while (j < SCALESTACK_SIZE-1 && flReturn.m_aiScaleStack[j+1] == 0)
						{
							TAssert(j+1 < SCALESTACK_SIZE-1);
							flReturn.m_aiScaleStack[j+1] += 999;
							j++;
						}

						flReturn.m_aiScaleStack[j+1]--;
						flReturn.m_aiScaleStack[i] += 1000;
					}
				}
				else if (!flReturn.m_bPositive)
				{
					int j = i;
					while (flReturn.m_aiScaleStack[i] > 0)
					{
						while (j < SCALESTACK_SIZE-1 && flReturn.m_aiScaleStack[j+1] == 0)
						{
							TAssert(j+1 < SCALESTACK_SIZE-1);
							flReturn.m_aiScaleStack[j+1] -= 999;
							j++;
						}

						flReturn.m_aiScaleStack[j+1]++;
						flReturn.m_aiScaleStack[i] -= 1000;
					}
				}
			}
		}
		else
			flReturn.m_aiScaleStack[i] = 0;
	}

	flReturn.m_bZero = m_bZero && u.m_bZero;
	
	CHECKSANITY(flReturn);

	return flReturn;
}

CScalableFloat CScalableFloat::operator+(const CScalableFloat& u) const
{
	if (m_bPositive == u.m_bPositive)
		return AddSimilar(u);
	else
		return AddDifferent(u);
}

CScalableFloat CScalableFloat::operator-(const CScalableFloat& u) const
{
	if (m_bZero)
		return -u;

	if (u.m_bZero)
		return *this;

	if (m_bPositive == u.m_bPositive)
		return AddDifferent(-u);
	else
		return AddSimilar(-u);
}

void CScalableFloat::operator+=(const CScalableFloat& u)
{
	if (m_bPositive == u.m_bPositive)
		*this = AddSimilar(u);
	else
		*this = AddDifferent(u);
}

void CScalableFloat::operator-=(const CScalableFloat& u)
{
	if (m_bPositive == u.m_bPositive)
		*this = AddDifferent(-u);
	else
		*this = AddSimilar(-u);
}

CScalableFloat CScalableFloat::operator*( const CScalableFloat& f ) const
{
	// Since I hard-coded the code below, this would have to be updated if I add a scale.
	TAssert(SCALESTACK_SIZE == SCALE_TERAMETER);

	float fll0 = m_aiScaleStack[0];
	float fll1 = m_aiScaleStack[1];
	float fll2 = m_aiScaleStack[2];
	float fll3 = m_aiScaleStack[3];
	float fll4 = m_aiScaleStack[4];
	float fll5 = m_aiScaleStack[5];

	float flr0 = f.m_aiScaleStack[0];
	float flr1 = f.m_aiScaleStack[1];
	float flr2 = f.m_aiScaleStack[2];
	float flr3 = f.m_aiScaleStack[3];
	float flr4 = f.m_aiScaleStack[4];
	float flr5 = f.m_aiScaleStack[5];

	float aflResults[11];
	aflResults[0] = fll0 * flr0;
	aflResults[1] = fll1 * flr0 + fll0 * flr1;
	aflResults[2] = fll2 * flr0 + fll1 * flr1 + fll0 * flr2;
	aflResults[3] = fll3 * flr0 + fll2 * flr1 + fll1 * flr2 + fll0 * flr3;
	aflResults[4] = fll4 * flr0 + fll3 * flr1 + fll2 * flr2 + fll1 * flr3 + fll0 * flr4;
	aflResults[5] = fll5 * flr0 + fll4 * flr1 + fll3 * flr2 + fll2 * flr3 + fll1 * flr4 + fll0 * flr5;
	aflResults[6] = fll5 * flr1 + fll4 * flr2 + fll3 * flr3 + fll2 * flr4 + fll1 * flr5;
	aflResults[7] = fll5 * flr2 + fll4 * flr3 + fll3 * flr4 + fll2 * flr5;
	aflResults[8] = fll5 * flr3 + fll4 * flr4 + fll3 * flr5;
	aflResults[9] = fll5 * flr4 + fll4 * flr5;
	aflResults[10] = fll5 * flr5;

	CScalableFloat flReturn;
	flReturn.m_bPositive = f.m_bPositive?m_bPositive:!m_bPositive;
	flReturn.m_bZero = true;

	for (size_t i = 0; i < 11; i++)
	{
		int j = i;
		float flValue = aflResults[j];

		if (flValue == 0)
			continue;

		// Subtract 1 for every scale below meters to keep the decimal in the right place.
		j -= (SCALE_METER-1);

		flReturn.m_bZero = false;

		if (m_bPositive)
		{
			while (flValue < 1)
			{
				flValue *= 1000;
				j--;
			}

			while (flValue > 1000)
			{
				flValue /= 1000;
				j++;
			}

			TAssert(j < SCALESTACK_SIZE);

			while (flValue > 0 && j >= 0)
			{
				flReturn.m_aiScaleStack[j] += (short)flValue;
				flValue -= (short)flValue;
				flValue *= 1000;
				flReturn.NormalizeStackPosition(j);
				j--;
			}
		}
		else
		{
			while (flValue > -1)
			{
				flValue *= 1000;
				j--;
			}

			while (flValue < -1000)
			{
				flValue /= 1000;
				j++;
			}

			TAssert(j < SCALESTACK_SIZE);

			while (flValue < 0 && j >= 0)
			{
				flReturn.m_aiScaleStack[j] += (short)flValue;
				flValue -= (short)flValue;
				flValue *= 1000;
				flReturn.NormalizeStackPosition(j);
				j--;
			}
		}
	}
	
	CHECKSANITY(flReturn);

	return flReturn;
}

CScalableFloat CScalableFloat::operator/( const CScalableFloat& f ) const
{
	if (f.m_bZero)
		return CScalableFloat();

	// I don't like doing this but there's really no way to do a proper division.
	float flDivide = f.GetUnits(SCALE_METER);

	if (flDivide == 0)
		return CScalableFloat();

	CScalableFloat flReturn;

	flReturn.m_bPositive = f.m_bPositive?m_bPositive:!m_bPositive;

	for (size_t i = 0; i < SCALESTACK_SIZE; i++)
	{
		if (m_aiScaleStack[i] == 0)
			continue;

		float flResult = m_aiScaleStack[i]/flDivide;
		int j = i;

		if (flReturn.m_bPositive)
		{
			while (flResult > 0 && flResult < 0.001f && j >= 0)
			{
				flResult *= 1000;
				j--;
			}

			while (flResult > 0 && j >= 0)
			{
				flReturn.m_aiScaleStack[j] += (short)flResult;
				flReturn.NormalizeStackPosition(j);
				flResult -= (short)flResult;
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

			while (flResult < 0 && j >= 0)
			{
				flReturn.m_aiScaleStack[j] = (short)flResult;
				flReturn.NormalizeStackPosition(j);
				flResult -= (short)flResult;
				flResult *= 1000;
				j--;
			}
		}
	}

	flReturn.m_bZero = m_bZero;
	
	CHECKSANITY(flReturn);

	return flReturn;
}

CScalableFloat CScalableFloat::operator*(float f) const
{
	if (f == 0)
		return CScalableFloat();

	if (m_bZero)
		return *this;

	if (f == 1)
		return *this;

	CScalableFloat flReturn;

	flReturn.m_bPositive = (f > 0)?m_bPositive:!m_bPositive;

	for (size_t i = 0; i < SCALESTACK_SIZE; i++)
	{
		float flValue = m_aiScaleStack[i] * f;

		if (flValue == 0)
			continue;

		int j = i;

		while (flValue < -1000 || flValue > 1000)
		{
			flValue /= 1000;
			j++;
		}

		while (((flReturn.m_bPositive && flValue > 0) || (!flReturn.m_bPositive && flValue < 0)) && j >= 0)
		{
			flReturn.m_aiScaleStack[j] += (short)flValue;
			while (flReturn.m_aiScaleStack[j] <= -1000 || flReturn.m_aiScaleStack[j] >= 1000)
				flReturn.NormalizeStackPosition(j);
			flValue -= (short)flValue;
			flValue *= 1000;
			j--;
		}
	}

	flReturn.m_bZero = false;
	
	CHECKSANITY(flReturn);

	return flReturn;
}

CScalableFloat CScalableFloat::operator/(float f) const
{
	TAssert(f != 0);
	if (f == 0)
		return CScalableFloat();

	if (m_bZero)
		return CScalableFloat();

	CScalableFloat flReturn;
	flReturn.m_bPositive = (f > 0)?m_bPositive:!m_bPositive;

	for (size_t i = 0; i < SCALESTACK_SIZE; i++)
	{
		float flValue = m_aiScaleStack[i] / f;

		int j = i;
		while (fabs(flValue) > 0 && j >= 0)
		{
			flReturn.m_aiScaleStack[j] += (short)flValue;
			flReturn.NormalizeStackPosition(j);
			flValue -= (short)flValue;
			flValue *= 1000;
			j--;
		}
	}

	flReturn.m_bZero = false;

	CHECKSANITY(flReturn);

	return flReturn;
}

CScalableFloat CScalableFloat::AddMultiple(const CScalableFloat& f, const CScalableFloat& g) const
{
	if (m_bZero)
	{
		if (f.m_bZero)
			return g;

		if (g.m_bZero)
			return f;

		return f + g;
	}

	if (f.m_bZero)
	{
		if (m_bZero)
			return g;

		if (g.m_bZero)
			return *this;

		return *this + g;
	}

	if (g.m_bZero)
	{
		if (m_bZero)
			return f;

		if (f.m_bZero)
			return *this;

		return *this + f;
	}

	CScalableFloat flReturn;

	for (int i = SCALESTACK_SIZE-1; i >= 0; i--)
	{
		flReturn.m_aiScaleStack[i] = m_aiScaleStack[i] + f.m_aiScaleStack[i] + g.m_aiScaleStack[i];

		int j = i;
		while ((flReturn.m_aiScaleStack[j] <= -1000 || flReturn.m_aiScaleStack[j] >= 1000) && j < SCALESTACK_SIZE-2)
		{
			while (flReturn.m_aiScaleStack[j] >= 1000)
			{
				flReturn.m_aiScaleStack[j] -= 1000;
				flReturn.m_aiScaleStack[j+1] += 1;
			}

			while (flReturn.m_aiScaleStack[j] <= -1000)
			{
				flReturn.m_aiScaleStack[j] += 1000;
				flReturn.m_aiScaleStack[j+1] -= 1;
			}

			j++;
		}
	}

	bool bLargest = true;
	for (int i = SCALESTACK_SIZE-1; i >= 0; i--)
	{
		if (bLargest)
		{
			if (flReturn.m_aiScaleStack[i])
			{
				flReturn.m_bPositive = flReturn.m_aiScaleStack[i] > 0;
				bLargest = false;
			}
		}
		else
		{
			if (flReturn.m_bPositive && flReturn.m_aiScaleStack[i] < 0)
			{
				flReturn.m_aiScaleStack[i] += 1000;
				flReturn.m_aiScaleStack[i+1] -= 1;
			}
			else if (!flReturn.m_bPositive && flReturn.m_aiScaleStack[i] > 0)
			{
				flReturn.m_aiScaleStack[i] -= 1000;
				flReturn.m_aiScaleStack[i+1] += 1;
			}
		}

		while (flReturn.m_aiScaleStack[i] > 1000 || flReturn.m_aiScaleStack[i] < -1000)
			flReturn.NormalizeStackPosition(i);
	}

	flReturn.m_bZero = m_bZero && f.m_bZero && g.m_bZero;

	CHECKSANITY(flReturn);

	return flReturn;
}

CScalableFloat CScalableFloat::AddMultiple(const CScalableFloat& f, const CScalableFloat& g, const CScalableFloat& h) const
{
	CScalableFloat flReturn;

	for (int i = SCALESTACK_SIZE-1; i >= 0; i--)
	{
		flReturn.m_aiScaleStack[i] = m_aiScaleStack[i] + f.m_aiScaleStack[i] + g.m_aiScaleStack[i] + h.m_aiScaleStack[i];

		int j = i;
		while ((flReturn.m_aiScaleStack[j] <= -1000 || flReturn.m_aiScaleStack[j] >= 1000) && j < SCALESTACK_SIZE-2)
		{
			while (flReturn.m_aiScaleStack[j] >= 1000)
			{
				flReturn.m_aiScaleStack[j] -= 1000;
				flReturn.m_aiScaleStack[j+1] += 1;
			}

			while (flReturn.m_aiScaleStack[j] <= -1000)
			{
				flReturn.m_aiScaleStack[j] += 1000;
				flReturn.m_aiScaleStack[j+1] -= 1;
			}

			j++;
		}
	}

	bool bLargest = true;
	for (int i = SCALESTACK_SIZE-1; i >= 0; i--)
	{
		if (bLargest)
		{
			if (flReturn.m_aiScaleStack[i])
			{
				flReturn.m_bPositive = flReturn.m_aiScaleStack[i] > 0;
				bLargest = false;
			}
		}
		else
		{
			if (flReturn.m_bPositive && flReturn.m_aiScaleStack[i] < 0)
			{
				while (flReturn.m_aiScaleStack[i] < 0)
				{
					int j = i;
					while (j < SCALESTACK_SIZE-1 && flReturn.m_aiScaleStack[j+1] == 0)
					{
						TAssert(j+1 < SCALESTACK_SIZE-1);
						flReturn.m_aiScaleStack[j+1] += 999;
						j++;
					}

					flReturn.m_aiScaleStack[i] += 1000;
					flReturn.m_aiScaleStack[j+1] -= 1;
				}
			}
			else if (!flReturn.m_bPositive && flReturn.m_aiScaleStack[i] > 0)
			{
				while (flReturn.m_aiScaleStack[i] > 0)
				{
					int j = i;
					while (j < SCALESTACK_SIZE-1 && flReturn.m_aiScaleStack[j+1] == 0)
					{
						TAssert(j+1 < SCALESTACK_SIZE-1);
						flReturn.m_aiScaleStack[j+1] -= 999;
						j++;
					}

					flReturn.m_aiScaleStack[i] -= 1000;
					flReturn.m_aiScaleStack[j+1] += 1;
				}
			}
		}

		while (flReturn.m_aiScaleStack[i] >= 1000 || flReturn.m_aiScaleStack[i] <= -1000)
			flReturn.NormalizeStackPosition(i);
	}

	flReturn.m_bZero = m_bZero && f.m_bZero && g.m_bZero && h.m_bZero;

	CHECKSANITY(flReturn);

	return flReturn;
}

void CScalableFloat::NormalizeStackPosition( int i )
{
	if (m_bPositive)
	{
		for (int j = i; j < SCALESTACK_SIZE-1; j++)
		{
			if (m_aiScaleStack[j] >= 1000)
			{
				m_aiScaleStack[j] -= 1000;
				m_aiScaleStack[j+1] += 1;
			}
			else
				break;
		}

		TAssert(m_aiScaleStack[SCALESTACK_SIZE-1] < 1000);
	}
	else
	{
		for (int j = i; j < SCALESTACK_SIZE-1; j++)
		{
			if (m_aiScaleStack[j] <= -1000)
			{
				m_aiScaleStack[j] += 1000;
				m_aiScaleStack[j+1] -= 1;
			}
			else
				break;
		}

		TAssert(m_aiScaleStack[SCALESTACK_SIZE-1] > -1000);
	}
}

void CScalableFloat::CheckSanity()
{
	for (int i = 0; i < SCALESTACK_SIZE; i++)
	{
		if (m_bZero)
		{
			TAssert(m_aiScaleStack[i] == 0);
		}
		else
		{
			if (m_bPositive)
			{
				TAssert(m_aiScaleStack[i] >= 0);
				TAssert(m_aiScaleStack[i] < 1000);
			}
			else
			{
				TAssert(m_aiScaleStack[i] <= 0);
				TAssert(m_aiScaleStack[i] > -1000);
			}
		}
	}
}

bool CScalableFloat::operator<(const CScalableFloat& u) const
{
	for (int i = SCALESTACK_SIZE-1; i >= 0; i--)
	{
		if (m_aiScaleStack[i] - u.m_aiScaleStack[i] == 0)
			continue;

		return m_aiScaleStack[i] < u.m_aiScaleStack[i];
	}

	return false;
}

bool CScalableFloat::operator>(const CScalableFloat& u) const
{
	for (int i = SCALESTACK_SIZE-1; i >= 0; i--)
	{
		if (m_aiScaleStack[i] - u.m_aiScaleStack[i] == 0)
			continue;

		return m_aiScaleStack[i] > u.m_aiScaleStack[i];
	}

	return false;
}

static float g_aflConversions[11] = 
{
	0.000000000000001f,
	0.000000000001f,
	0.000000001f,
	0.000001f,
	0.001f,
	1.0f,
	1000.0f,
	1000000.0f,
	1000000000.0f,
	1000000000000.0f,
	1000000000000000.0f,
};

double CScalableFloat::ConvertUnits(double flUnits, scale_t eFrom, scale_t eTo)
{
	if (eFrom == eTo)
		return flUnits;

	if (flUnits == 0)
		return 0;

	TAssert( eTo >= SCALE_MILLIMETER && eTo <= SCALE_HIGHEST );
	if (eTo == SCALE_NONE)
		return 0;

	if (eTo < SCALE_NONE)
		return 0;

	if (eTo > SCALE_HIGHEST)
		return 0;

	TAssert( SCALE_HIGHEST+5 <= 11 );
	return flUnits * g_aflConversions[eFrom-eTo+5];
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

DoubleVector CScalableVector::GetUnits(scale_t eScale) const
{
	DoubleVector vecResult = Vector(0,0,0);

	vecResult.x = x.GetUnits(eScale);
	vecResult.y = y.GetUnits(eScale);
	vecResult.z = z.GetUnits(eScale);

	return vecResult;
}

bool CScalableVector::IsZero() const
{
	return x.IsZero() && y.IsZero() && z.IsZero();
}

bool CScalableVector::IsZero()
{
	return x.IsZero() && y.IsZero() && z.IsZero();
}

CScalableFloat CScalableVector::Length() const
{
	float flX = x.GetUnits(SCALE_METER);
	float flY = y.GetUnits(SCALE_METER);
	float flZ = z.GetUnits(SCALE_METER);
	float f = flX*flX + flY*flY + flZ*flZ;
	CScalableFloat r(sqrt(f), SCALE_METER);

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
