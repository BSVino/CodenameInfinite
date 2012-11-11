#pragma once

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

#define CHUNK_TERRAIN_LODS 4

class CTerrainChunk
{
	friend class CTerrainChunkManager;

public:
										CTerrainChunk(class CTerrainChunkManager* pManager, size_t iChunk, size_t iTerrain, size_t iLump, const DoubleVector2D& vecMin, const DoubleVector2D& vecMax);
										~CTerrainChunk();

public:
	void								Initialize();

	void								Think();
	void								GenerateTerrain();

	void								Render();

	bool                                IsExtraPhysicsEntGround(size_t iEnt) const;

	void                                GetCoordinates(unsigned short& x, unsigned short& y) const;
	size_t                              GetTerrain() const;
	size_t                              GetLump() const;
	bool                                IsGeneratingTerrain() const { return m_bGeneratingTerrain; }
	bool                                IsGenerationDone() const { return m_bGenerationDone; }
	size_t                              GetPhysicsEntity() const { return m_iPhysicsEntity; }
	bool                                HasPhysicsEntity() const { return m_iPhysicsEntity != ~0; }
	DoubleVector                        GetLocalCenter() const { return m_vecLocalCenter; }
	double                              GetRadius() const { return m_flRadius; }
	DoubleMatrix4x4                     GetPlanetToChunk() const { return m_mPlanetToChunk; }
	DoubleMatrix4x4                     GetChunkToPlanet() const { return m_mChunkToPlanet; }
	size_t                              GetParity() const { return m_iParity; }

protected:
	class CTerrainChunkManager*			m_pManager;
	size_t                              m_iChunk;
	size_t                              m_iParity;

	size_t                              m_iTerrain;
	size_t                              m_iLump;

	DoubleVector2D                      m_vecMin;
	DoubleVector2D                      m_vecMax;
	size_t                              m_iX;
	size_t                              m_iY;

	DoubleVector                        m_vecLocalCenter;	// Center in planet space
	double                              m_flRadius;

	TemplateAABB<double>				m_aabbBounds;
	DoubleMatrix4x4                     m_mPlanetToChunk;
	DoubleMatrix4x4                     m_mChunkToPlanet;

	size_t                              m_iTerrainVBO;
	CTerrainLOD                         m_aiTerrainLODs[CHUNK_TERRAIN_LODS][CHUNK_TERRAIN_LODS];

	bool                                m_bGeneratingTerrain;
	bool                                m_bGenerationDone;
	tvector<float>                      m_aflVBODrop;
	tvector<CTerrainLODDrop>            m_aLODDrops;

	bool                                m_bLoadIntoPhysics;
	tvector<float>                      m_aflPhysicsVerts;
	tvector<int>                        m_aiPhysicsVerts;
	size_t								m_iPhysicsMesh;
	size_t								m_iPhysicsEntity;

private:
	static size_t                       s_iChunks;
};

class CTerrainChunkManager
{
	friend class CTerrainChunk;

public:
										CTerrainChunkManager(class CPlanet* pPlanet);

public:
	void								AddChunk(size_t iLump, const DoubleVector2D& vecChunkMin, const DoubleVector2D& vecChunkMax);
	void								RemoveChunk(size_t iChunk);

	void								LumpRemoved(size_t iLump);

	CTerrainChunk*						GetChunk(size_t iChunk) const;
	size_t								GetNumChunks() const { return m_apChunks.size(); }

	bool                                HasGroupCenter() const { return m_bHaveGroupCenter; }
	const DoubleMatrix4x4&                 GetPlanetToGroupCenterTransform() const { return m_mPlanetToGroup; }
	const DoubleMatrix4x4&                 GetGroupCenterToPlanetTransform() const { return m_mGroupToPlanet; }

	void								Think();
	void                                FindCenterChunk();
	void                                AddNearbyChunks();
	void								GenerateChunk(size_t iChunk);
	void								Render();

	bool                                IsGenerating() const;

private:
	void								RemoveChunkNoLock(size_t iChunk);

protected:
	class CPlanet*						m_pPlanet;
	tvector<CTerrainChunk*>             m_apChunks;

	double								m_flNextChunkCheck;

	size_t                      m_iActiveChunks;

	// A "group" is a group of chunks, grouped for physics purposes.
	bool                        m_bHaveGroupCenter;
	DoubleMatrix4x4             m_mPlanetToGroup; // Really just the transforms of the center chunk of the group.
	DoubleMatrix4x4             m_mGroupToPlanet;

	static class CParallelizer*         s_pChunkGenerator;
};
