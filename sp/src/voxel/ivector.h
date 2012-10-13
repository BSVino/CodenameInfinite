#pragma once

#include <vector.h>

class IVector
{
public:
	IVector();
	IVector(int x, int y, int z);

public:
	IVector    operator*(int i) const;
	IVector    operator+(const IVector& r) const;

	// For using as the key of a tmap
	bool       operator<(const IVector& r) const;

	IVector    FindChunk() const;
	IVector    FindChunkCoordinates() const;
	Vector     GetVoxelSpaceCoordinates() const;

public:
	int x;
	int y;
	int z;
};
