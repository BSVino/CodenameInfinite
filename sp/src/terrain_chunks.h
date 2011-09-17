#ifndef SP_TERRAIN_CHUNKS_H
#define SP_TERRAIN_CHUNKS_H

#include <EASTL/vector.h>
#include <EASTL/map.h>

#include <geometry.h>

#include "planet_terrain.h"
#include "sp_common.h"

namespace raytrace {
	class CRaytracer;
};

class CTerrainChunk
{
public:
										CTerrainChunk(CTerrainQuadTreeBranch* pBranch);
										~CTerrainChunk();

public:
	void								Render(class CRenderingContext* c);

	bool								Raytrace(const DoubleVector& vecStart, const DoubleVector& vecEnd, CCollisionResult& tr);

	CTerrainQuadTreeBranch*				GetBranch() { return m_pBranch; }

protected:
	CTerrainQuadTreeBranch*				m_pBranch;
	raytrace::CRaytracer*				m_pRaytracer;
};

class CTerrainChunkManager
{
public:
										CTerrainChunkManager(class CPlanet* pPlanet);

public:
	void								AddChunk(CTerrainQuadTreeBranch* pBranch);
	void								RemoveChunk(CTerrainQuadTreeBranch* pBranch);
	size_t								FindChunk(CTerrainQuadTreeBranch* pBranch);
	CTerrainChunk*						GetChunk(size_t iChunk);

	void								ProcessChunkRendering(CTerrainQuadTreeBranch* pBranch);
	void								Render();

protected:
	class CPlanet*						m_pPlanet;
	eastl::vector<CTerrainChunk*>		m_apChunks;
	eastl::map<CTerrainQuadTreeBranch*, size_t>	m_apBranchChunks;
	eastl::map<scale_t, eastl::vector<CTerrainChunk*> >	m_apRenderChunks;
};

#endif
