#ifndef _SIMPLEX_H
#define _SIMPLEX_H

#include <mtrand.h>

template <class F>
class CSimplexNoise
{
public:
					CSimplexNoise();
					CSimplexNoise(size_t iSeed);

public:
	void			Init(size_t iSeed);

	F				Noise(F x, F y);

private:
	inline F		Grad(int hash, F x, F y)
	{
		int h = hash&7;
		F u = h<4?x:y;
		F v = h<4?y:x;
		return ((h&1)?-u:u) + ((h&2)? -2.0f*v : 2.0f*v);
	}

	inline F		Fade(F t)
	{
		return t*t*t*(t*(t*6-15)+10);
	}

	inline int		Floor(F x)
	{
		if (x >= 0)
			return (int)x;
		else
			return (int)x-1;
	}

	inline F		Lerp(F t, F a, F b)
	{
		return a + t*(b-a);
	}

private:
	unsigned char	m_aiRand[512];
};

template <class F>
inline CSimplexNoise<F>::CSimplexNoise()
{
}

template <class F>
inline CSimplexNoise<F>::CSimplexNoise(size_t iSeed)
{
	Init(iSeed);
}

template <class F>
inline void CSimplexNoise<F>::Init(size_t iSeed)
{
	mtsrand(iSeed);
	for (size_t i = 0; i < 256; i++)
		m_aiRand[i] = (unsigned char)(mtrand()%255);
	memcpy(&m_aiRand[256], &m_aiRand[0], 256);
}

template <class F>
inline F CSimplexNoise<F>::Noise(F x, F y)
{
    int ix0, iy0, ix1, iy1;
    F fx0, fy0, fx1, fy1;
    F s, t, nx0, nx1, n0, n1;

    ix0 = Floor(x);
    iy0 = Floor(y);
    fx0 = x - ix0;
    fy0 = y - iy0;
    fx1 = fx0 - 1.0f;
    fy1 = fy0 - 1.0f;
    ix1 = (ix0 + 1) & 0xff;
    iy1 = (iy0 + 1) & 0xff;
    ix0 = ix0 & 0xff;
    iy0 = iy0 & 0xff;
    
    t = Fade( fy0 );
    s = Fade( fx0 );

    nx0 = Grad(m_aiRand[ix0 + m_aiRand[iy0]], fx0, fy0);
    nx1 = Grad(m_aiRand[ix0 + m_aiRand[iy1]], fx0, fy1);
    n0 = Lerp( t, nx0, nx1 );

    nx0 = Grad(m_aiRand[ix1 + m_aiRand[iy0]], fx1, fy0);
    nx1 = Grad(m_aiRand[ix1 + m_aiRand[iy1]], fx1, fy1);
    n1 = Lerp(t, nx0, nx1);

    return 0.507f * ( Lerp( s, n0, n1 ) );
}

#endif
