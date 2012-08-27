#ifndef SP_TERRAIN_CHUNKS_H
#define SP_TERRAIN_CHUNKS_H

#include <EASTL/vector.h>
#include <EASTL/map.h>

#include <geometry.h>

#include "planet_terrain.h"
#include "sp_common.h"

class CTerrainChunk
{
public:
										CTerrainChunk(class CTerrainChunkManager* pManager, CTerrainQuadTreeBranch* pBranch);
										~CTerrainChunk();

public:
	void								Think();
	void								BuildBranchToDepth();

	void								Render(class CRenderingContext* c);

	CTerrainQuadTreeBranch*				GetBranch() { return m_pBranch; }

	size_t								GetDepth() { return m_iDepth; }

protected:
	class CTerrainChunkManager*			m_pManager;
	CTerrainQuadTreeBranch*				m_pBranch;

	size_t								m_iDepth;
	size_t								m_iMaxDepth;
};

class CTerrainChunkManager
{
	friend class CTerrainChunk;

public:
										CTerrainChunkManager(class CPlanet* pPlanet);

public:
	void								AddChunk(CTerrainQuadTreeBranch* pBranch);
	void								RemoveChunk(CTerrainQuadTreeBranch* pBranch);
	size_t								FindChunk(CTerrainQuadTreeBranch* pBranch) const;
	CTerrainChunk*						GetChunk(size_t iChunk) const;
	size_t								GetNumChunks() const { return m_apChunks.size(); }

	void								Think();
	void								ProcessChunkRendering(CTerrainQuadTreeBranch* pBranch);
	void								Render();

protected:
	class CPlanet*						m_pPlanet;
	tvector<CTerrainChunk*>             m_apChunks;
	tmap<CTerrainQuadTreeBranch*, size_t> m_apBranchChunks;
	tmap<scale_t, tvector<CTerrainChunk*> > m_apRenderChunks;
};

#endif
