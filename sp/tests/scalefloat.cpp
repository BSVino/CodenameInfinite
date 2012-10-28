#include <stdio.h>

#include <matrix.h>
#include <quaternion.h>
#include <common.h>
#include <maths.h>
#include <tstring.h>

#include <tinker/shell.h>

#include <sp_common.h>

#define FULL_TEST

void test_scalefloat()
{
	CScalableFloat f;

	TAssert(f.IsZero());

	f = CScalableFloat(7);

	TAssert(f.GetUnits(SCALE_METER) == 7);

	f = CScalableFloat((double)14, SCALE_KILOMETER);

	TAssert(f.GetUnits(SCALE_METER) == 14000);

	f = CScalableFloat((float)14, SCALE_KILOMETER);

	TAssert(f.GetUnits(SCALE_METER) == 14000);
	TAssert(f.IsPositive());

	f = -f;

	TAssert(f.GetUnits(SCALE_METER) == -14000);
	TAssert(f.IsNegative());

	CScalableFloat t(1);

	for (size_t i = 0; i < 100000; i++)
	{
		f += t;
		TAssert(f == CScalableFloat(-14000.0 + i + 1, SCALE_METER));
	}

	TAssert(f == CScalableFloat(86.0, SCALE_KILOMETER));

	for (size_t i = 0; i < 100000; i++)
	{
		f -= t;
		TAssert(f == CScalableFloat(86000.0 - i - 1, SCALE_METER));
	}

	TAssert(f == CScalableFloat(-14.0, SCALE_KILOMETER));

	TAssert(CScalableFloat() * 2.0f == CScalableFloat());
	TAssert(CScalableFloat() / 2.0f == CScalableFloat());

	TAssert(CScalableFloat(1.0) * 2.0f == CScalableFloat(2.0));
	TAssert(CScalableFloat(1.0) / 2.0f == CScalableFloat(0.5));

	TAssert(CScalableFloat(0.1f) * 2.0f == CScalableFloat(0.2f));
	TAssert(CScalableFloat(0.1f) / 2.0f == CScalableFloat(0.05f));

	TAssert(CScalableFloat(1000) * 2.0f == CScalableFloat(2000));
	TAssert(CScalableFloat(1000) / 2.0f == CScalableFloat(500));

	TAssert(CScalableFloat(1000) < CScalableFloat(2000));
	TAssert(CScalableFloat(2000) > CScalableFloat(1000));
	TAssert(!(CScalableFloat(1000) > CScalableFloat(2000)));
	TAssert(!(CScalableFloat(2000) < CScalableFloat(1000)));

	TAssert(CScalableFloat(1000) < 2000.0f);
	TAssert(CScalableFloat(2000) > 1000.0f);
	TAssert(!(CScalableFloat(1000) > 2000.0f));
	TAssert(!(CScalableFloat(2000) < 1000.0f));

	TAssert(CScalableFloat(1000) == CScalableFloat(1000));
	TAssert(CScalableFloat(1000) != CScalableFloat(2000));
	TAssert(!(CScalableFloat(1000) == CScalableFloat(2000)));
	TAssert(!(CScalableFloat(1000) != CScalableFloat(1000)));

	// Test that overflow works properly.
	TAssert(CScalableFloat(500.0, SCALE_HIGHEST) * CScalableFloat(3.0) == CScalableFloat(1500.0, SCALE_HIGHEST));
	TAssert(CScalableFloat(1500.0, SCALE_HIGHEST) / CScalableFloat(2.0) == CScalableFloat(750.0, SCALE_HIGHEST));

	TAssert(CScalableFloat(500.0, SCALE_HIGHEST) * 3.0 == CScalableFloat(1500.0, SCALE_HIGHEST));
	TAssert(CScalableFloat(1500.0, SCALE_HIGHEST) / 2.0 == CScalableFloat(750.0, SCALE_HIGHEST));

	TAssert(CScalableFloat(1000.0, SCALE_HIGHEST).AddMultiple(CScalableFloat(-500.0, SCALE_HIGHEST), CScalableFloat(-500.0, SCALE_HIGHEST), CScalableFloat()) == CScalableFloat());

	TAssert(CScalableFloat(1000.0, SCALE_HIGHEST) == CScalableFloat(1000.0, SCALE_HIGHEST));
	TAssert(CScalableFloat(1100.0, SCALE_HIGHEST) == CScalableFloat(1100.0, SCALE_HIGHEST));
	TAssert(CScalableFloat(11000.0, SCALE_HIGHEST) == CScalableFloat(11000.0, SCALE_HIGHEST));
	TAssert(CScalableFloat(1100.0, SCALE_HIGHEST) > CScalableFloat(1000.0, SCALE_HIGHEST));
	TAssert(CScalableFloat(1000.0, SCALE_HIGHEST) < CScalableFloat(1100.0, SCALE_HIGHEST));
	TAssert(CScalableFloat(-1100.0, SCALE_HIGHEST) < CScalableFloat(-1000.0, SCALE_HIGHEST));
	TAssert(CScalableFloat(-1000.0, SCALE_HIGHEST) > CScalableFloat(-1100.0, SCALE_HIGHEST));
}
