#pragma once

#include <vector.h>

class CTreeAddress
{
public:
	CTreeAddress()
	{
		iTerrain = ~0;
	}

public:
	bool IsValid() const
	{
		if (iTerrain == ~0)
			return false;

		return true;
	}

	void Reset()
	{
		iTerrain = ~0;
	}

	bool operator!=(const CTreeAddress& r)
	{
		if (iTerrain != r.iTerrain)
			return true;

		if (vecMin != r.vecMin)
			return true;

		if (iTree != r.iTree)
			return true;

		return false;
	}

public:
	size_t         iTerrain;
	DoubleVector2D vecMin;
	size_t         iTree;
};
