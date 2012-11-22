#pragma once

#include <vector.h>
#include <tvector.h>
#include <tmap.h>

#include "voxel/voxel_tree.h"

class CLumpVoxelManager
{
	friend class CVoxelTree;

public:
	CLumpVoxelManager(class CPlanet* pPlanet);

public:
	void          LoadChunk(class CTerrainChunk* pChunk);
	void          UnloadChunk(class CTerrainChunk* pChunk);

	const CVoxelTree* GetLumpVoxel(size_t iTerrain, const DoubleVector2D& vecMin) const;
	CVoxelTree*   GetLumpVoxel(size_t iTerrain, const DoubleVector2D& vecMin);
	CVoxelTree*   AddLumpVoxel(class CTerrainLump* pLump);

	class CPlanet*  GetPlanet() const { return m_pPlanet; }

private:
	class CPlanet*  m_pPlanet;

	tmap<CLumpAddress, CVoxelTree> m_aLumpVoxels;
};
