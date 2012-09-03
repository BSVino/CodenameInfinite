#ifndef SP_TERRAIN_CHUNKS_H
#define SP_TERRAIN_CHUNKS_H

#include <tvector.h>
#include <tmap.h>

#include <geometry.h>

#include "planet_terrain.h"
#include "sp_common.h"

class CChunkGenerationJob
{
public:
	class CTerrainChunkManager*	pManager;
	size_t                      iChunk;
};

class CTerrainChunk
{
	friend class CTerrainChunkManager;

public:
										CTerrainChunk(class CTerrainChunkManager* pManager, size_t iChunk, size_t iTerrain, const DoubleVector2D& vecMin, const DoubleVector2D& vecMax);
										~CTerrainChunk();

public:
	void								Initialize();

	void								Think();
	void								GenerateTerrain();

	void								Render();

protected:
	class CTerrainChunkManager*			m_pManager;
	size_t                              m_iChunk;

	size_t                              m_iTerrain;

	DoubleVector2D                      m_vecMin;
	DoubleVector2D                      m_vecMax;

	DoubleVector                        m_vecLocalCenter;	// Center in planet space

	TemplateAABB<double>				m_aabbBounds;

	size_t								m_iLowResTerrainVBO;
	size_t								m_iLowResTerrainIBO;
	size_t								m_iLowResTerrainIBOSize;

	bool                                m_bGeneratingLowRes;
	tvector<float>                      m_aflLowResDrop;
	tvector<unsigned int>               m_aiLowResDrop;
};

class CTerrainChunkManager
{
	friend class CTerrainChunk;

public:
										CTerrainChunkManager(class CPlanet* pPlanet);

public:
	void								AddChunk(size_t iTerrain, const DoubleVector2D& vecChunkMin, const DoubleVector2D& vecChunkMax);
	void								RemoveChunk(size_t iChunk);

	CTerrainChunk*						GetChunk(size_t iChunk) const;
	size_t								GetNumChunks() const { return m_apChunks.size(); }

	void								Think();
	void                                AddNearbyChunks();
	void								GenerateChunk(size_t iChunk);
	void								Render();

private:
	void								RemoveChunkNoLock(size_t iChunk);

protected:
	class CPlanet*						m_pPlanet;
	tvector<CTerrainChunk*>             m_apChunks;

	double								m_flNextChunkCheck;

	static class CParallelizer*         s_pChunkGenerator;
};

#endif
