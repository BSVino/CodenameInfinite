#include "ivector.h"

#include "voxel_chunk.h"

IVector::IVector()
{
	x = y = z = 0;
}

IVector::IVector(int X, int Y, int Z)
{
	x = X;
	y = Y;
	z = Z;
}

IVector IVector::operator*(int i) const
{
	return IVector(x*i, y*i, z*i);
}

IVector IVector::operator+(const IVector& r) const
{
	return IVector(x+r.x, y+r.y, z+r.z);
}

IVector IVector::operator-(const IVector& r) const
{
	return IVector(x-r.x, y-r.y, z-r.z);
}

bool IVector::operator<(const IVector& r) const
{
	if (x < r.x)
		return true;

	if (x > r.x)
		return false;

	if (y < r.y)
		return true;

	if (y > r.y)
		return false;

	if (z < r.z)
		return true;

	return false;
}

bool IVector::operator==(const IVector& r) const
{
	if (x != r.x)
		return false;

	if (y != r.y)
		return false;

	if (z != r.z)
		return false;

	return true;
}

bool IVector::operator!=(const IVector& r) const
{
	if (x != r.x)
		return true;

	if (y != r.y)
		return true;

	if (z != r.z)
		return true;

	return false;
}

IVector IVector::FindChunk() const
{
	int xpos = ((x<0)?-x-1:x);
	int xmod = xpos%CHUNK_SIZE;
	int xorigin = xpos - xmod;
	int xchunk = ((x<0)?-1:1)*((xorigin / CHUNK_SIZE)+((x<0)?1:0));

	int ypos = ((y<0)?-y-1:y);
	int ymod = ypos%CHUNK_SIZE;
	int yorigin = ypos - ymod;
	int ychunk = ((y<0)?-1:1)*((yorigin / CHUNK_SIZE)+((y<0)?1:0));

	int zpos = ((z<0)?-z-1:z);
	int zmod = zpos%CHUNK_SIZE;
	int zorigin = zpos - zmod;
	int zchunk = ((z<0)?-1:1)*((zorigin / CHUNK_SIZE)+((z<0)?1:0));

	return IVector(xchunk, ychunk, zchunk);
}

IVector IVector::FindChunkCoordinates() const
{
	IVector vecChunk = FindChunk();

	IVector vecResult(x-vecChunk.x*CHUNK_SIZE, y-vecChunk.y*CHUNK_SIZE, z-vecChunk.z*CHUNK_SIZE);

	TAssertNoMsg(vecResult.x >= 0 && vecResult.x < CHUNK_SIZE);
	TAssertNoMsg(vecResult.y >= 0 && vecResult.y < CHUNK_SIZE);
	TAssertNoMsg(vecResult.z >= 0 && vecResult.z < CHUNK_SIZE);

	return vecResult;
}

Vector IVector::GetVoxelSpaceCoordinates() const
{
	return Vector((float)x, (float)y, (float)z);
}
