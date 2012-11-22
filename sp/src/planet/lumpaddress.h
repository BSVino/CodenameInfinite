#pragma once

#include <vector.h>

class CLumpAddress
{
public:
	CLumpAddress()
	{
		iTerrain = ~0;
	}

public:
	size_t         iTerrain;
	DoubleVector2D vecMin;

public:
	bool operator<(const CLumpAddress& b) const
	{
		if (iTerrain < b.iTerrain)
			return true;

		if (iTerrain > b.iTerrain)
			return false;

		if (vecMin.x < b.vecMin.x)
			return true;

		if (vecMin.x > b.vecMin.x)
			return false;

		if (vecMin.y < b.vecMin.y)
			return true;

		return false;
	}
};
