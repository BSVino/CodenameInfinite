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
	m_flRemainder = 0;
	m_flOverflow = 0;
}

CScalableFloat::CScalableFloat(float flUnits, scale_t eScale)
{
	memset(&m_aiScaleStack[0], 0, SCALESTACK_SIZE*sizeof(short));

	m_bZero = true;
	m_flRemainder = 0;
	m_flOverflow = 0;

	Construct(flUnits, eScale);
}

CScalableFloat::CScalableFloat(double flUnits, scale_t eScale)
{
	memset(&m_aiScaleStack[0], 0, SCALESTACK_SIZE*sizeof(short));
	m_bZero = true;
	m_flRemainder = 0;
	m_flOverflow = 0;

	Construct(flUnits, eScale);
}

CScalableFloat::CScalableFloat(float flMeters)
{
	memset(&m_aiScaleStack[0], 0, SCALESTACK_SIZE*sizeof(short));
	m_bZero = true;
	m_flRemainder = 0;
	m_flOverflow = 0;

	Construct(flMeters, SCALE_METER);
}

void CScalableFloat::Construct(double flUnits, scale_t eScale)
{
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

		while (flUnits >= 1000)
		{
			flUnits /= 1000;
			i++;
		}

		if (i >= SCALESTACK_SIZE)
		{
			int k = i;
			while (k-- > SCALESTACK_SIZE)
				flUnits *= 1000;
			if (flUnits < 2000000000)
			{
				m_flOverflow += (long)flUnits;
				flUnits -= (long)flUnits;
				flUnits *= 1000;
			}
			else
			{
				m_flOverflow += flUnits;
				flUnits = 0;
			}
			i = SCALESTACK_SIZE-1;
		}

		while (flUnits > 0 && i >= 0)
		{
			m_aiScaleStack[i] = (short)flUnits;
			flUnits -= m_aiScaleStack[i];
			flUnits *= 1000;
			i--;
		}

		m_flRemainder = flUnits/1000;
	}
	else
	{
		while (flUnits > -1)
		{
			flUnits *= 1000;
			i--;
		}

		while (flUnits <= -1000)
		{
			flUnits /= 1000;
			i++;
		}

		if (i >= SCALESTACK_SIZE)
		{
			int k = i;
			while (k-- > SCALESTACK_SIZE)
				flUnits *= 1000;
			if (flUnits > -2000000000)
			{
				m_flOverflow += (long)flUnits;
				flUnits -= (long)flUnits;
				flUnits *= 1000;
			}
			else
			{
				m_flOverflow += flUnits;
				flUnits = 0;
			}
			i = SCALESTACK_SIZE-1;
		}

		while (flUnits < 0 && i >= 0)
		{
			m_aiScaleStack[i] = (short)flUnits;
			flUnits -= m_aiScaleStack[i];
			flUnits *= 1000;
			i--;
		}

		m_flRemainder = flUnits/1000;
	}

	CHECKSANITY(*this);
}

double CScalableFloat::GetUnits(scale_t eScale) const
{
	if (m_bZero)
		return 0;

	if (eScale == SCALE_RENDER)
		eScale = SCALE_METER;

	double flResult = 0;
	for (size_t i = 0; i < SCALESTACK_SIZE; i++)
		flResult += ConvertUnits(m_aiScaleStack[i], (scale_t)(i+1), eScale);

	flResult += ConvertUnits(m_flRemainder, (scale_t)(SCALE_NONE+1), eScale);
	flResult += ConvertUnits(m_flOverflow*1000, SCALE_HIGHEST, eScale);

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

	f.m_flRemainder = -m_flRemainder;
	f.m_flOverflow = -m_flOverflow;
	f.m_bPositive = !m_bPositive;
	f.m_bZero = m_bZero;

	CHECKSANITY(f);

	return f;
}

CScalableFloat CScalableFloat::AddSimilar(const CScalableFloat& u) const
{
	TAssertNoMsg(m_bPositive == u.m_bPositive);

	CScalableFloat flReturn;
	flReturn.m_bPositive = m_bPositive;
	flReturn.m_bZero = m_bZero && u.m_bZero;

	flReturn.m_flOverflow = m_flOverflow + u.m_flOverflow;

	for (size_t i = 0; i < SCALESTACK_SIZE; i++)
	{
		flReturn.m_aiScaleStack[i] += m_aiScaleStack[i] + u.m_aiScaleStack[i];
		flReturn.NormalizeStackPosition(i);
	}

	flReturn.m_flRemainder = m_flRemainder + u.m_flRemainder;
	flReturn.NormalizeRemainder();

	CHECKSANITY(flReturn);

	return flReturn;
}

CScalableFloat CScalableFloat::AddDifferent(const CScalableFloat& u) const
{
	TAssertNoMsg(m_bPositive != u.m_bPositive);

	CScalableFloat flReturn;
	flReturn.m_bPositive = m_bPositive;

	bool bHighest = true;

	flReturn.m_flOverflow = m_flOverflow + u.m_flOverflow;
	if (flReturn.m_flOverflow > 0)
	{
		bHighest = false;
		flReturn.m_bPositive = true;
	}
	else if (flReturn.m_flOverflow < 0)
	{
		bHighest = false;
		flReturn.m_bPositive = false;
	}

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
							flReturn.m_aiScaleStack[j+1] += 999;
							j++;
						}

						if (j == SCALESTACK_SIZE-1)
							flReturn.m_flOverflow -= 1;
						else
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
							flReturn.m_aiScaleStack[j+1] -= 999;
							j++;
						}

						if (j == SCALESTACK_SIZE-1)
							flReturn.m_flOverflow += 1;
						else
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
	
	flReturn.m_flRemainder = m_flRemainder + u.m_flRemainder;

	// It's all zero
	if (bHighest)
	{
		if (flReturn.m_flRemainder == 0)
			flReturn.m_bZero = true;
		else
		{
			flReturn.m_bZero = false;
			flReturn.m_bPositive = flReturn.m_flRemainder > 0;
		}
	}

	flReturn.NormalizeRemainder();

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
	if (m_bZero)
	{
		*this = u;
		return;
	}

	if (u.m_bZero)
		return;

	if (m_bPositive == u.m_bPositive)
		*this = AddSimilar(u);
	else
		*this = AddDifferent(u);
}

void CScalableFloat::operator-=(const CScalableFloat& u)
{
	if (m_bZero)
	{
		*this = -u;
		return;
	}

	if (u.m_bZero)
		return;

	if (m_bPositive == u.m_bPositive)
		*this = AddDifferent(-u);
	else
		*this = AddSimilar(-u);
}

CScalableFloat CScalableFloat::operator*( const CScalableFloat& f ) const
{
	// Since I hard-coded the code below, this would have to be updated if I add a scale.
	TAssertNoMsg(SCALESTACK_SIZE == SCALE_TERAMETER);

	const int iResults = 13;
	double aflResults[iResults];

	{
		double fll0 = (double)m_aiScaleStack[0] + m_flRemainder;
		double fll1 = m_aiScaleStack[1];
		double fll2 = m_aiScaleStack[2];
		double fll3 = m_aiScaleStack[3];
		double fll4 = m_aiScaleStack[4];
		double fll5 = m_aiScaleStack[5];
		double fll6 = m_flOverflow;

		double flr0 = (double)f.m_aiScaleStack[0] + f.m_flRemainder;
		double flr1 = f.m_aiScaleStack[1];
		double flr2 = f.m_aiScaleStack[2];
		double flr3 = f.m_aiScaleStack[3];
		double flr4 = f.m_aiScaleStack[4];
		double flr5 = f.m_aiScaleStack[5];
		double flr6 = f.m_flOverflow;

		aflResults[0] = fll0 * flr0;
		aflResults[1] = fll1 * flr0 + fll0 * flr1;
		aflResults[2] = fll2 * flr0 + fll1 * flr1 + fll0 * flr2;
		aflResults[3] = fll3 * flr0 + fll2 * flr1 + fll1 * flr2 + fll0 * flr3;
		aflResults[4] = fll4 * flr0 + fll3 * flr1 + fll2 * flr2 + fll1 * flr3 + fll0 * flr4;
		aflResults[5] = fll5 * flr0 + fll4 * flr1 + fll3 * flr2 + fll2 * flr3 + fll1 * flr4 + fll0 * flr5;
		aflResults[6] = fll6 * flr0 + fll5 * flr1 + fll4 * flr2 + fll3 * flr3 + fll2 * flr4 + fll1 * flr5 + fll0 * flr6;
		aflResults[7] = fll6 * flr1 + fll5 * flr2 + fll4 * flr3 + fll3 * flr4 + fll2 * flr5 + fll1 * flr6;
		aflResults[8] = fll6 * flr2 + fll5 * flr3 + fll4 * flr4 + fll3 * flr5 + fll2 * flr6;
		aflResults[9] = fll6 * flr3 + fll5 * flr4 + fll4 * flr5 + fll3 * flr6;
		aflResults[10] = fll6 * flr4 + fll5 * flr5 + fll4 * flr6;
		aflResults[11] = fll6 * flr5 + fll5 * flr6;
		aflResults[12] = fll6 * flr6;
	}

	CScalableFloat flReturn;
	flReturn.m_bPositive = f.m_bPositive?m_bPositive:!m_bPositive;
	flReturn.m_bZero = true;

	for (size_t i = 0; i < iResults; i++)
	{
		int j = i;
		double flValue = aflResults[j];

		if (flValue == 0)
			continue;

		// Subtract 1 for every scale below meters to keep the decimal in the right place.
		j -= (SCALE_METER-1);

		flReturn.m_bZero = false;

		if (flReturn.m_bPositive)
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

			if (j >= SCALESTACK_SIZE)
			{
				int k = j;
				while (k-- > SCALESTACK_SIZE)
					flValue *= 1000;
				if (flValue < 2000000000 && flValue > -2000000000)
				{
					flReturn.m_flOverflow += (long)flValue;
					flValue -= (long)flValue;
					flValue *= 1000;
				}
				else
				{
					flReturn.m_flOverflow += flValue;
					flValue = 0;
				}
				j = SCALESTACK_SIZE-1;
			}

			while (flValue > 0 && j >= 0)
			{
				flReturn.m_aiScaleStack[j] += (short)flValue;
				flValue -= (short)flValue;
				flValue *= 1000;
				flReturn.NormalizeStackPosition(j);
				j--;
			}

			while (j < -1)
			{
				flValue /= 1000;
				j++;
			}

			flReturn.m_flRemainder += flValue/1000;
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

			if (j >= SCALESTACK_SIZE)
			{
				int k = j;
				while (k-- > SCALESTACK_SIZE)
					flValue *= 1000;
				if (flValue < 2000000000 && flValue > -2000000000)
				{
					flReturn.m_flOverflow += (long)flValue;
					flValue -= (long)flValue;
					flValue *= 1000;
				}
				else
				{
					flReturn.m_flOverflow += flValue;
					flValue = 0;
				}
				j = SCALESTACK_SIZE-1;
			}

			while (flValue < 0 && j >= 0)
			{
				flReturn.m_aiScaleStack[j] += (short)flValue;
				flValue -= (short)flValue;
				flValue *= 1000;
				flReturn.NormalizeStackPosition(j);
				j--;
			}

			while (j < -1)
			{
				flValue /= 1000;
				j++;
			}

			flReturn.m_flRemainder += flValue/1000;
		}
	}
	
	flReturn.NormalizeRemainder();

	CHECKSANITY(flReturn);

	return flReturn;
}

CScalableFloat CScalableFloat::operator/( const CScalableFloat& f ) const
{
	if (f.m_bZero)
	{
		TAssertNoMsg(!f.m_bZero);
		return CScalableFloat();
	}

	// I don't like doing this but there's really no way to do a proper division.
	double flDivide = (double)f.GetUnits(SCALE_METER);

	if (flDivide == 0)
		return CScalableFloat();

	CScalableFloat flReturn;

	flReturn.m_bPositive = f.m_bPositive?m_bPositive:!m_bPositive;

	for (size_t i = 0; i <= SCALESTACK_SIZE+1; i++)
	{
		double flResult;
		int j = i;
		if (i == SCALESTACK_SIZE)
		{
			// Really this is a hack but I want to borrow the code in this loop for the remainder as well.
			flResult = m_flRemainder/flDivide;
			j = 0;
		}
		else if (i == SCALESTACK_SIZE+1)
		{
			// And the overflow.
			flResult = m_flOverflow/flDivide;
			j = SCALESTACK_SIZE;
		}
		else
		{
			if (m_aiScaleStack[i] == 0)
				continue;

			flResult = m_aiScaleStack[i]/flDivide;
			j = i;
		}

		if (flReturn.m_bPositive)
		{
			while (flResult > 0 && flResult < 0.001f && j >= 0)
			{
				flResult *= 1000;
				j--;
			}

			while (flResult > 1000)
			{
				flResult /= 1000;
				j++;
			}

			if (j >= SCALESTACK_SIZE)
			{
				int k = j;
				while (k-- > SCALESTACK_SIZE)
					flResult *= 1000;
				if (flResult < 2000000000 && flResult > -2000000000)
				{
					flReturn.m_flOverflow += (long)flResult;
					flResult -= (long)flResult;
					flResult *= 1000;
				}
				else
				{
					flReturn.m_flOverflow += flResult;
					flResult = 0;
				}
				j = SCALESTACK_SIZE-1;
			}

			while (flResult > 0 && j >= 0)
			{
				flReturn.m_aiScaleStack[j] += (short)flResult;
				flReturn.NormalizeStackPosition(j);
				flResult -= (short)flResult;
				flResult *= 1000;
				j--;
			}

			flReturn.m_flRemainder += flResult/1000;
		}
		else
		{
			while (flResult < 0 && flResult > -0.001f && j >= 0)
			{
				flResult *= 1000;
				j--;
			}

			while (flResult < -1000)
			{
				flResult /= 1000;
				j++;
			}

			if (j >= SCALESTACK_SIZE)
			{
				int k = j;
				while (k-- > SCALESTACK_SIZE)
					flResult *= 1000;
				if (flResult > -2000000000)
				{
					flReturn.m_flOverflow += (long)flResult;
					flResult -= (long)flResult;
					flResult *= 1000;
				}
				else
				{
					flReturn.m_flOverflow += flResult;
					flResult = 0;
				}
				j = SCALESTACK_SIZE-1;
			}

			while (flResult < 0 && j >= 0)
			{
				flReturn.m_aiScaleStack[j] += (short)flResult;
				flReturn.NormalizeStackPosition(j);
				flResult -= (short)flResult;
				flResult *= 1000;
				j--;
			}

			flReturn.m_flRemainder += flResult/1000;
		}
	}

	flReturn.NormalizeRemainder();

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

	double flOverflow = m_flOverflow*f;
	double flOverflowFractional;
	if (flOverflow < 2000000000 && flOverflow > -2000000000)
	{
		flOverflowFractional = flOverflow - (long)flOverflow;
		flOverflow = (long)flOverflow;
	}
	else
	{
		flOverflowFractional = 0;
	}

	CScalableFloat flReturn(flOverflowFractional*1000, SCALE_HIGHEST);
	flReturn.m_flOverflow = flOverflow;

	flReturn.m_bPositive = (f > 0)?m_bPositive:!m_bPositive;

	flReturn.m_bZero = false;
	
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

		if (j >= SCALESTACK_SIZE)
		{
			int k = j;
			while (k-- > SCALESTACK_SIZE)
				flValue *= 1000;
			flReturn.m_flOverflow += (long)flValue;
			flValue -= (long)flValue;
			flValue *= 1000;
			j--;
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

		flReturn.m_flRemainder += flValue/1000;
	}

	flReturn.m_flRemainder += m_flRemainder * f;
	flReturn.NormalizeRemainder();

	CHECKSANITY(flReturn);

	return flReturn;
}

CScalableFloat CScalableFloat::operator/(float f) const
{
	TAssertNoMsg(f != 0);
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

	flReturn.m_flRemainder = m_flRemainder + f.m_flRemainder + g.m_flRemainder;
	if (flReturn.m_flRemainder < -1 || flReturn.m_flRemainder > 1)
	{
		flReturn.m_aiScaleStack[0] += (int)flReturn.m_flRemainder;
		flReturn.m_flRemainder -= (int)flReturn.m_flRemainder;
	}

	for (int i = SCALESTACK_SIZE-1; i >= 0; i--)
	{
		flReturn.m_aiScaleStack[i] += m_aiScaleStack[i] + f.m_aiScaleStack[i] + g.m_aiScaleStack[i];

		int j = i;
		while ((flReturn.m_aiScaleStack[j] <= -1000 || flReturn.m_aiScaleStack[j] >= 1000) && j < SCALESTACK_SIZE-1)
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

	if (bLargest)
		flReturn.m_bPositive = flReturn.m_flRemainder > 0;

	flReturn.NormalizeRemainder();

	CHECKSANITY(flReturn);

	return flReturn;
}

CScalableFloat CScalableFloat::AddMultiple(const CScalableFloat& f, const CScalableFloat& g, const CScalableFloat& h) const
{
	CScalableFloat flReturn;

	flReturn.m_flRemainder = m_flRemainder + f.m_flRemainder + g.m_flRemainder + h.m_flRemainder;
	if (flReturn.m_flRemainder < -1 || flReturn.m_flRemainder > 1)
	{
		flReturn.m_aiScaleStack[0] += (int)flReturn.m_flRemainder;
		flReturn.m_flRemainder -= (int)flReturn.m_flRemainder;
	}

	for (int i = SCALESTACK_SIZE-1; i >= 0; i--)
	{
		flReturn.m_aiScaleStack[i] += m_aiScaleStack[i] + f.m_aiScaleStack[i] + g.m_aiScaleStack[i] + h.m_aiScaleStack[i];

		int j = i;
		while ((flReturn.m_aiScaleStack[j] <= -1000 || flReturn.m_aiScaleStack[j] >= 1000) && j < SCALESTACK_SIZE-1)
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
						flReturn.m_aiScaleStack[j+1] += 999;
						j++;
					}

					if (j == SCALESTACK_SIZE-1)
						flReturn.m_flOverflow -= 1;
					else
						flReturn.m_aiScaleStack[j+1]--;

					flReturn.m_aiScaleStack[i] += 1000;
				}
			}
			else if (!flReturn.m_bPositive && flReturn.m_aiScaleStack[i] > 0)
			{
				while (flReturn.m_aiScaleStack[i] > 0)
				{
					int j = i;
					while (j < SCALESTACK_SIZE-1 && flReturn.m_aiScaleStack[j+1] == 0)
					{
						flReturn.m_aiScaleStack[j+1] -= 999;
						j++;
					}

					if (j == SCALESTACK_SIZE-1)
						flReturn.m_flOverflow += 1;
					else
						flReturn.m_aiScaleStack[j+1]++;

					flReturn.m_aiScaleStack[i] -= 1000;
				}
			}
		}

		while (flReturn.m_aiScaleStack[i] >= 1000 || flReturn.m_aiScaleStack[i] <= -1000)
			flReturn.NormalizeStackPosition(i);
	}

	flReturn.m_bZero = m_bZero && f.m_bZero && g.m_bZero && h.m_bZero;

	if (bLargest)
		flReturn.m_bPositive = flReturn.m_flRemainder > 0;

	flReturn.NormalizeRemainder();

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

		if (m_aiScaleStack[SCALESTACK_SIZE-1] >= 1000)
		{
			m_aiScaleStack[SCALESTACK_SIZE-1] -= 1000;
			m_flOverflow += 1;
		}
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

		if (m_aiScaleStack[SCALESTACK_SIZE-1] <= -1000)
		{
			m_aiScaleStack[SCALESTACK_SIZE-1] += 1000;
			m_flOverflow -= 1;
		}
	}
}

void CScalableFloat::NormalizeRemainder()
{
	if (m_flRemainder >= 1 || m_flRemainder <= -1)
	{
		m_aiScaleStack[0] += (short)m_flRemainder;
		while (m_aiScaleStack[0] >= 1000 || m_aiScaleStack[0] <= -1000)
			NormalizeStackPosition(0);
		m_flRemainder -= (short)m_flRemainder;
	}

	if (m_flRemainder < FLT_EPSILON && m_flRemainder > -FLT_EPSILON)
	{
		m_flRemainder = 0;
		return;
	}

	if (!m_bZero && m_bPositive && m_flRemainder < 0)
	{
		m_flRemainder += 1;
		m_aiScaleStack[0] -= 1;

		int j = 0;
		while (m_aiScaleStack[j] < 0)
		{
			while (j < SCALESTACK_SIZE-1 && m_aiScaleStack[j+1] == 0)
			{
				m_aiScaleStack[j+1] += 999;
				j++;
			}

			if (j == SCALESTACK_SIZE-1)
				m_flOverflow -= 1;
			else
				m_aiScaleStack[j+1]--;

			m_aiScaleStack[0] += 1000;
		}
	}
	else if (!m_bZero && !m_bPositive && m_flRemainder > 0)
	{
		m_flRemainder -= 1;
		m_aiScaleStack[0] += 1;

		int j = 0;
		while (m_aiScaleStack[0] > 0)
		{
			while (j < SCALESTACK_SIZE-1 && m_aiScaleStack[j+1] == 0)
			{
				m_aiScaleStack[j+1] -= 999;
				j++;
			}

			if (j == SCALESTACK_SIZE-1)
				m_flOverflow += 1;
			else
				m_aiScaleStack[j+1]++;

			m_aiScaleStack[0] -= 1000;
		}
	}
}

void CScalableFloat::CheckSanity()
{
	for (int i = 0; i < SCALESTACK_SIZE; i++)
	{
		if (m_bZero)
		{
			TAssertNoMsg(m_aiScaleStack[i] == 0);
		}
		else
		{
			if (m_bPositive)
			{
				TAssertNoMsg(m_aiScaleStack[i] >= 0);
				TAssertNoMsg(m_aiScaleStack[i] < 1000);
			}
			else
			{
				TAssertNoMsg(m_aiScaleStack[i] <= 0);
				TAssertNoMsg(m_aiScaleStack[i] > -1000);
			}
		}
	}

	if (m_bZero)
	{
		TAssertNoMsg(m_flRemainder == 0);
		TAssertNoMsg(m_flOverflow == 0);
	}
	else if (m_bPositive)
	{
		TAssertNoMsg(m_flRemainder >= 0);
		TAssertNoMsg(m_flRemainder < 1);
		TAssertNoMsg(m_flOverflow >= 0);
	}
	else
	{
		TAssertNoMsg(m_flRemainder <= 0);
		TAssertNoMsg(m_flRemainder > -1);
		TAssertNoMsg(m_flOverflow <= 0);
	}
}

bool CScalableFloat::operator==(const CScalableFloat& u) const
{
	if (m_flOverflow != u.m_flOverflow)
		return false;

	double flDifference = 0;
	for (int i = SCALESTACK_SIZE-1; i >= 0; i--)
	{
		if (m_aiScaleStack[i] != u.m_aiScaleStack[i])
			flDifference += CScalableFloat::ConvertUnits(m_aiScaleStack[i] - u.m_aiScaleStack[i], SCALESTACK_SCALE(i), SCALE_METER);
	}

	flDifference += (m_flRemainder - u.m_flRemainder)/1000;
	float flEp = 0.001f;
	return fabs(flDifference) < flEp;
}

bool CScalableFloat::operator!=(const CScalableFloat& u) const
{
	return (!(*this == u));
}

bool CScalableFloat::operator<(const CScalableFloat& u) const
{
	if (m_flOverflow != 0 || u.m_flOverflow != 0)
		return m_flOverflow < u.m_flOverflow;

	for (int i = SCALESTACK_SIZE-1; i >= 0; i--)
	{
		if (m_aiScaleStack[i] - u.m_aiScaleStack[i] == 0)
			continue;

		return m_aiScaleStack[i] < u.m_aiScaleStack[i];
	}

	return (m_flRemainder < u.m_flRemainder);
}

bool CScalableFloat::operator>(const CScalableFloat& u) const
{
	if (m_flOverflow != 0 || u.m_flOverflow != 0)
		return m_flOverflow > u.m_flOverflow;

	for (int i = SCALESTACK_SIZE-1; i >= 0; i--)
	{
		if (m_aiScaleStack[i] - u.m_aiScaleStack[i] == 0)
			continue;

		return m_aiScaleStack[i] > u.m_aiScaleStack[i];
	}

	return (m_flRemainder > u.m_flRemainder);
}

bool CScalableFloat::operator<(float flMeters) const
{
	return GetUnits(SCALE_METER) < flMeters;
}

bool CScalableFloat::operator>(float flMeters) const
{
	return GetUnits(SCALE_METER) > flMeters;
}

bool CScalableFloat::operator<=(const CScalableFloat& u) const
{
	if (m_flOverflow != 0 || u.m_flOverflow != 0)
		return m_flOverflow <= u.m_flOverflow;

	for (int i = SCALESTACK_SIZE-1; i >= 0; i--)
	{
		if (m_aiScaleStack[i] - u.m_aiScaleStack[i] == 0)
			continue;

		return m_aiScaleStack[i] <= u.m_aiScaleStack[i];
	}

	return (m_flRemainder <= u.m_flRemainder);
}

bool CScalableFloat::operator>=(const CScalableFloat& u) const
{
	if (m_flOverflow != 0 || u.m_flOverflow != 0)
		return m_flOverflow >= u.m_flOverflow;

	for (int i = SCALESTACK_SIZE-1; i >= 0; i--)
	{
		if (m_aiScaleStack[i] - u.m_aiScaleStack[i] == 0)
			continue;

		return m_aiScaleStack[i] >= u.m_aiScaleStack[i];
	}

	return (m_flRemainder >= u.m_flRemainder);
}

CScalableFloat::operator double() const
{
	return GetUnits(SCALE_METER);
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

	TAssertNoMsg( eTo >= SCALE_MILLIMETER && eTo <= SCALE_HIGHEST );
	if (eTo == SCALE_NONE)
		return 0;

	if (eTo < SCALE_NONE)
		return 0;

	if (eTo > SCALE_HIGHEST)
		return 0;

	TAssertNoMsg( SCALE_HIGHEST+5 <= 11 );
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

CScalableVector::CScalableVector(Vector vecMeters)
{
	x = CScalableFloat(vecMeters.x, SCALE_METER);
	y = CScalableFloat(vecMeters.y, SCALE_METER);
	z = CScalableFloat(vecMeters.z, SCALE_METER);
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

void CScalableVector::Normalize()
{
	*this = CScalableVector(*this).Normalized();
}

CScalableFloat CScalableVector::Length() const
{
	CScalableFloat f = x*x + y*y + z*z;
	CScalableFloat r(sqrt(f.GetUnits(SCALE_METER)), SCALE_METER);

	return r;
}

CScalableFloat CScalableVector::LengthSqr() const
{
	return x*x + y*y + z*z;
}

CScalableFloat CScalableVector::Dot(const CScalableVector& v) const
{
	return x*v.x + y*v.y + z*v.z;
}

CScalableVector CScalableVector::Cross(const CScalableVector& v) const
{
	return CScalableVector(y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x);
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

CScalableVector CScalableVector::operator*( const CScalableVector& v ) const
{
	CScalableVector vecReturn;
	for (size_t i = 0; i < 3; i++)
		vecReturn[i] = Index(i) * v.Index(i);

	return vecReturn;
}

void CScalableVector::operator+=(const CScalableVector& u)
{
	x += u.x;
	y += u.y;
	z += u.z;
}

void CScalableVector::operator-=(const CScalableVector& u)
{
	x -= u.x;
	y -= u.y;
	z -= u.z;
}

CScalableFloat CScalableVector::Index(int i) const
{
	if (i == 0)
		return x;
	if (i == 1)
		return y;
	if (i == 2)
		return z;

	TAssertNoMsg(false);
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

	TAssertNoMsg(false);
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

bool CScalableVector::operator==(const CScalableVector& u) const
{
	return (x == u.x && y == u.y && z == u.z);
}

CScalableVector::operator Vector() const
{
	return GetUnits(SCALE_METER);
}

CScalableMatrix::CScalableMatrix(const Vector& vecForward, const Vector& vecUp, const Vector& vecRight, const CScalableVector& vecPosition)
{
	SetForwardVector(vecForward);
	SetUpVector(vecUp);
	SetRightVector(vecRight);
	SetTranslation(vecPosition);
}

CScalableMatrix::CScalableMatrix(const EAngle& angDirection, const CScalableVector& vecPosition)
{
	SetAngles(angDirection);
	SetTranslation(vecPosition);
}

CScalableMatrix::CScalableMatrix(const Matrix4x4& mOther)
{
	for (size_t x = 0; x < 3; x++)
	{
		for (size_t y = 0; y < 3; y++)
			m[x][y] = mOther.m[x][y];
	}

	SetTranslation(mOther.GetTranslation());
}

CScalableMatrix::CScalableMatrix(const DoubleMatrix4x4& mOther)
{
	for (size_t x = 0; x < 3; x++)
	{
		for (size_t y = 0; y < 3; y++)
			m[x][y] = (float)mOther.m[x][y];
	}

	SetTranslation(CScalableVector(mOther.GetTranslation(), SCALE_METER));
}

void CScalableMatrix::Identity()
{
	memset(this, 0, sizeof(m));

	m[0][0] = m[1][1] = m[2][2] = 1;

	SetTranslation(CScalableVector());
}

bool CScalableMatrix::IsIdentity()
{
	if (CScalableMatrix() == *this)
		return true;

	return false;
}

void CScalableMatrix::SetAngles(const EAngle& angDir)
{
	Matrix4x4 t;
	t.SetAngles(angDir);

	m[0][0] = t.m[0][0];
	m[0][1] = t.m[0][1];
	m[0][2] = t.m[0][2];
	m[1][0] = t.m[1][0];
	m[1][1] = t.m[1][1];
	m[1][2] = t.m[1][2];
	m[2][0] = t.m[2][0];
	m[2][1] = t.m[2][1];
	m[2][2] = t.m[2][2];
}

EAngle CScalableMatrix::GetAngles() const
{
	Matrix4x4 m(GetForwardVector(), GetUpVector(), GetRightVector());

	return m.GetAngles();
}

void CScalableMatrix::SetRotation(float flAngle, const Vector& v)
{
	Matrix4x4 t;
	t.SetRotation(flAngle, v);

	m[0][0] = t.m[0][0];
	m[0][1] = t.m[0][1];
	m[0][2] = t.m[0][2];
	m[1][0] = t.m[1][0];
	m[1][1] = t.m[1][1];
	m[1][2] = t.m[1][2];
	m[2][0] = t.m[2][0];
	m[2][1] = t.m[2][1];
	m[2][2] = t.m[2][2];
}

void CScalableMatrix::SetRotation(const Quaternion& q)
{
	Matrix4x4 t;
	t.SetRotation(q);

	m[0][0] = t.m[0][0];
	m[0][1] = t.m[0][1];
	m[0][2] = t.m[0][2];
	m[1][0] = t.m[1][0];
	m[1][1] = t.m[1][1];
	m[1][2] = t.m[1][2];
	m[2][0] = t.m[2][0];
	m[2][1] = t.m[2][1];
	m[2][2] = t.m[2][2];
}

void CScalableMatrix::SetOrientation(const Vector& v, const Vector& vecUp)
{
	Matrix4x4 t;
	t.SetOrientation(v, vecUp);

	m[0][0] = t.m[0][0];
	m[0][1] = t.m[0][1];
	m[0][2] = t.m[0][2];
	m[1][0] = t.m[1][0];
	m[1][1] = t.m[1][1];
	m[1][2] = t.m[1][2];
	m[2][0] = t.m[2][0];
	m[2][1] = t.m[2][1];
	m[2][2] = t.m[2][2];
}

CScalableMatrix CScalableMatrix::AddTranslation(const CScalableVector& v)
{
	CScalableMatrix r;
	r.SetTranslation(v);
	(*this) *= r;

	return *this;
}

CScalableMatrix CScalableMatrix::AddAngles(const EAngle& a)
{
	CScalableMatrix r;
	r.SetAngles(a);
	(*this) *= r;

	return *this;
}

CScalableMatrix CScalableMatrix::operator+=(const CScalableVector& v)
{
	mt[0] += v.x;
	mt[1] += v.y;
	mt[2] += v.z;

	return *this;
}

CScalableMatrix CScalableMatrix::operator+=(const EAngle& a)
{
	CScalableMatrix r;
	r.SetAngles(a);
	(*this) *= r;

	return *this;
}

CScalableMatrix CScalableMatrix::operator*(const CScalableMatrix& t) const
{
	CScalableMatrix r;

	// [a b c d][A B C D]   [aA+bE+cI+dM
	// [e f g h][E F G H] = [eA+fE+gI+hM ...
	// [i j k l][I J K L]
	// [m n o p][M N O P]

	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
			r.m[i][j] = m[0][j]*t.m[i][0] + m[1][j]*t.m[i][1] + m[2][j]*t.m[i][2];
	}

	r.mt[0] = (t.mt[0]*m[0][0]).AddMultiple(t.mt[1]*m[1][0], t.mt[2]*m[2][0], mt[0]);
	r.mt[1] = (t.mt[0]*m[0][1]).AddMultiple(t.mt[1]*m[1][1], t.mt[2]*m[2][1], mt[1]);
	r.mt[2] = (t.mt[0]*m[0][2]).AddMultiple(t.mt[1]*m[1][2], t.mt[2]*m[2][2], mt[2]);

	return r;
}

CScalableMatrix CScalableMatrix::operator*=(const CScalableMatrix& t)
{
	*this = (*this)*t;

	return *this;
}

CScalableVector CScalableMatrix::operator*(const CScalableVector& v) const
{
	// [a b c x][X] 
	// [d e f y][Y] = [aX+bY+cZ+x dX+eY+fZ+y gX+hY+iZ+z]
	// [g h i z][Z]
	//          [1]

	CScalableVector vecResult;
	vecResult.x = (v.x * m[0][0]).AddMultiple(v.y * m[1][0], v.z * m[2][0], mt[0]);
	vecResult.y = (v.x * m[0][1]).AddMultiple(v.y * m[1][1], v.z * m[2][1], mt[1]);
	vecResult.z = (v.x * m[0][2]).AddMultiple(v.y * m[1][2], v.z * m[2][2], mt[2]);
	return vecResult;
}

CScalableVector CScalableMatrix::TransformVector(const CScalableVector& v) const
{
	// [a b c][X] 
	// [d e f][Y] = [aX+bY+cZ dX+eY+fZ gX+hY+iZ]
	// [g h i][Z]

	CScalableVector vecResult;
	vecResult.x = (v.x * m[0][0]).AddMultiple(v.y * m[1][0], v.z * m[2][0]);
	vecResult.y = (v.x * m[0][1]).AddMultiple(v.y * m[1][1], v.z * m[2][1]);
	vecResult.z = (v.x * m[0][2]).AddMultiple(v.y * m[1][2], v.z * m[2][2]);
	return vecResult;
}

bool CScalableMatrix::operator==(const CScalableMatrix& t) const
{
	float flEp = 0.000001f;
	for (size_t x = 0; x < 3; x++)
	{
		for (size_t y = 0; y < 3; y++)
		{
			if (fabs(m[x][y] - t.m[x][y]) > flEp)
				return false;
		}
	}

	if (mt.x != t.mt.x)
		return false;
	if (mt.y != t.mt.y)
		return false;
	if (mt.z != t.mt.z)
		return false;

	return true;
}

bool CScalableMatrix::operator!=(const CScalableMatrix& t) const
{
	float flEp = 0.000001f;
	for (size_t x = 0; x < 3; x++)
	{
		for (size_t y = 0; y < 3; y++)
		{
			if (fabs(m[x][y] - t.m[x][y]) > flEp)
				return true;
		}
	}

	if (mt.x != t.mt.x)
		return true;
	if (mt.y != t.mt.y)
		return true;
	if (mt.z != t.mt.z)
		return true;

	return false;
}

// Not a true inversion, only works if the matrix is a translation/rotation matrix.
void CScalableMatrix::InvertRT()
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

CScalableMatrix CScalableMatrix::InvertedRT() const
{
	CScalableMatrix t = *this;

	t.InvertRT();

	return t;
}

Vector CScalableMatrix::GetRow(int i) const
{
	return Vector(m[i][0], m[i][1], m[i][2]);
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

void CScalableMatrix::SetForwardVector(const Vector& v)
{
	m[0][0] = v.x;
	m[0][1] = v.y;
	m[0][2] = v.z;
}

void CScalableMatrix::SetUpVector(const Vector& v)
{
	m[1][0] = v.x;
	m[1][1] = v.y;
	m[1][2] = v.z;
}

void CScalableMatrix::SetRightVector(const Vector& v)
{
	m[2][0] = v.x;
	m[2][1] = v.y;
	m[2][2] = v.z;
}

DoubleMatrix4x4 CScalableMatrix::GetUnits(scale_t eScale) const
{
	DoubleMatrix4x4 r;
	r.SetForwardVector(DoubleVector(m[0][0], m[0][1], m[0][2]));
	r.SetUpVector(DoubleVector(m[1][0], m[1][1], m[1][2]));
	r.SetRightVector(DoubleVector(m[2][0], m[2][1], m[2][2]));
	r.SetTranslation(mt.GetUnits(eScale));
	return r;
}

CScalableMatrix::operator Quaternion() const
{
	Matrix4x4 r;
	r.Init(m[0][0], m[0][1], m[0][2], 0, m[1][0], m[1][1], m[1][2], 0, m[2][0], m[2][1], m[2][2], 0, 0, 0, 0, 1);
	Quaternion q(r);
	return q;
}

CScalableMatrix::operator Matrix4x4() const
{
	return GetUnits(SCALE_METER);
}

bool LineSegmentIntersectsSphere(const CScalableVector& v1, const CScalableVector& v2, const CScalableVector& s, const CScalableFloat& flRadius, CScalableCollisionResult& tr)
{
	CScalableVector vecLine = v2 - v1;
	CScalableVector vecSphere = v1 - s;

	if (vecLine.IsZero())
	{
		if (vecSphere.Length() < flRadius)
		{
			tr.bHit = true;
			tr.vecHit = v1;
			tr.flFraction = 0;
			tr.vecNormal = vecSphere.Normalized();
			tr.bStartInside = true;
			return true;
		}
		else
			return false;
	}

	CScalableFloat flA = vecLine.LengthSqr();
	CScalableFloat flB = vecSphere.Dot(vecLine) * 2.0f;
	CScalableFloat flC1 = s.LengthSqr() + v1.LengthSqr();
	CScalableFloat flC2 = (s.Dot(v1)*2.0f);
	CScalableFloat flC = flC1 - flC2 - flRadius*flRadius;

	CScalableFloat flBB4AC = flB*flB - flA*flC*4.0f;
	if (flBB4AC < CScalableFloat())
		return false;

	CScalableFloat flSqrt(sqrt(flBB4AC.GetUnits(SCALE_METER)), SCALE_METER);
	CScalableFloat flPlus = (-flB + flSqrt)/(flA*2.0f);
	CScalableFloat flMinus = (-flB - flSqrt)/(flA*2.0f);

	CScalableFloat fl1(1.0f, SCALE_METER);
	bool bPlusBelow0 = flPlus < CScalableFloat();
	bool bMinusBelow0 = flMinus < CScalableFloat();
	bool bPlusAbove1 = flPlus > fl1;
	bool bMinusAbove1 = flMinus > fl1;

	// If both are above 1 or below 0, then we're not touching the sphere.
	if (bMinusBelow0 && bPlusBelow0 || bPlusAbove1 && bMinusAbove1)
		return false;

	if (bMinusBelow0 && bPlusAbove1)
	{
		// We are inside the sphere.
		tr.vecHit = v1;
		tr.vecNormal = (v1 - s).Normalized();
		tr.flFraction = 0;
		tr.bStartInside = true;
		return true;
	}

	if (bMinusAbove1 && bPlusBelow0)
	{
		// We are inside the sphere. Is this even possible? I dunno. I'm putting an assert here to see.
		// If it's still here later that means no.
		TAssertNoMsg(false);
		tr.vecHit = v1;
		tr.vecNormal = (v1 - s).Normalized();
		return true;
	}

	// If flPlus is below 1 and flMinus is below 0 that means we started our trace inside the sphere and we're heading out.
	// Don't intersect with the sphere in this case so that things on the inside can get out without getting stuck.
	if (bMinusBelow0 && !bPlusAbove1)
	{
		tr.bStartInside = true;
		return false;
	}

	// So at this point, flMinus is between 0 and 1, and flPlus is above 1.

	float flFraction = (float)flMinus.GetUnits(SCALE_METER);
	if (tr.flFraction < flFraction)
		return false;

	tr.bHit = true;
	tr.flFraction = flFraction;

	// In any other case, we intersect with the sphere and we use the flMinus value as the intersection point.
	CScalableFloat flDistance = vecLine.Length();
	CScalableVector vecDirection = vecLine / flDistance;

	tr.vecHit = v1 + vecDirection * (flDistance * flFraction);

	// Oftentimes we are slightly stuck inside the sphere. Pull us out a little bit.
	CScalableVector vecDifference = tr.vecHit - s;
	CScalableFloat flDifferenceLength = vecDifference.Length();
	tr.vecNormal = vecDifference / flDifferenceLength;
	if (flDifferenceLength < flRadius)
		tr.vecHit += tr.vecNormal * ((flRadius-flDifferenceLength) + CScalableFloat(0.1f, SCALE_MILLIMETER));
	TAssertNoMsg((tr.vecHit - s).Length() >= flRadius);

	return true;
}

bool LineSegmentIntersectsTriangle(const CScalableVector& s0, const CScalableVector& s1, const CScalableVector& v0, const CScalableVector& v1, const CScalableVector& v2, CScalableCollisionResult& tr)
{
	CScalableVector u = v1 - v0;
	CScalableVector v = v2 - v0;
	CScalableVector n = u.Cross(v);

	CScalableVector w0 = s0 - v0;

	CScalableFloat a = -n.Dot(w0);
	CScalableFloat b = n.Dot(s1-s0);

	CScalableFloat ep = 1e-4f;

	CScalableFloat flZero(0);
	CScalableFloat flOne(1);

	if ((b<flZero?-b:b) < ep)
	{
		if (a.IsZero())			// Segment is parallel
		{
			tr.bHit = true;
			tr.flFraction = 0;
			tr.vecHit = v0;
			tr.vecNormal = (v1-v0).Normalized().Cross((v2-v0).Normalized()).Normalized();
			return true;	// Segment is inside plane
		}
		else
			return false;	// Segment is somewhere else
	}

	CScalableFloat r = a/b;
	if (r < flZero)
		return false;		// Segment goes away from the triangle
	if (r > flOne)
		return false;		// Segment goes away from the triangle

	float flFraction = (float)r.GetUnits(SCALE_METER);
	if (tr.flFraction < flFraction)
		return false;

	CScalableVector vecPoint = s0 + (s1-s0)*r;

	CScalableFloat uu = u.Dot(u);
	CScalableFloat uv = u.Dot(v);
	CScalableFloat vv = v.Dot(v);
	CScalableVector w = vecPoint - v0;
	CScalableFloat wu = w.Dot(u);
	CScalableFloat wv = w.Dot(v);

	CScalableFloat D = uv * uv - uu * vv;

	CScalableFloat s = (uv * wv - vv * wu) / D;
	if (s <= ep || s >= flOne)		// Intersection point is outside the triangle
		return false;

	CScalableFloat t = (uv * wu - uu * wv) / D;
	if (t <= ep || (s+t) >= flOne)	// Intersection point is outside the triangle
		return false;

	tr.bHit = true;
	tr.flFraction = flFraction;
	tr.vecHit = vecPoint;
	tr.vecNormal = (v1-v0).Normalized().Cross((v2-v0).Normalized()).Normalized();

	return true;
}

