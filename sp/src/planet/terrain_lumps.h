#pragma once

#include <tvector.h>
#include <tmap.h>

#include <geometry.h>

#include "planet_terrain.h"
#include "sp_common.h"

class CLumpGenerationJob
{
public:
	class CTerrainLumpManager*	pManager;
	size_t                      iLump;
};

class CTerrainLump
{
	friend class CTerrainLumpManager;
	friend class CTerrainChunkManager;
	friend class CTerrainChunk;

public:
										CTerrainLump(class CTerrainLumpManager* pManager, size_t iLump, size_t iTerrain, const DoubleVector2D& vecMin, const DoubleVector2D& vecMax);
										~CTerrainLump();

public:
	void								Initialize();

	void								Think();
	void								GenerateTerrain();
	void								RebuildIndices();

	void								Render();

	bool                                FindApproximateElevation(const DoubleVector& vec3DLocal, float& flElevation) const;

	void                                GetCoordinates(unsigned short& x, unsigned short& y) const;
	size_t                              GetTerrain() const;
	bool                                IsGeneratingLowRes() const { return m_bGeneratingLowRes; }
	DoubleVector                        GetLocalCenter() const { return m_vecLocalCenter; }
	double                              GetRadius() const { return m_flRadius; }

protected:
	class CTerrainLumpManager*			m_pManager;
	size_t                              m_iLump;

	size_t                              m_iTerrain;

	DoubleVector2D                      m_vecMin;
	DoubleVector2D                      m_vecMax;
	size_t                              m_iX;
	size_t                              m_iY;

	DoubleVector                        m_vecLocalCenter;	// Center in planet space
	double                              m_flRadius;

	TemplateAABB<double>				m_aabbBounds;
	DoubleMatrix4x4                     m_mPlanetToLump;
	DoubleMatrix4x4                     m_mLumpToPlanet;

	size_t								m_iLowResTerrainVBO;
	size_t								m_iLowResTerrainIBO;
	size_t								m_iLowResTerrainIBOSize;

	bool                                m_bKDTreeAvailable;
	tvector<CKDPointTreeNode>           m_aKDNodes;
	tvector<CKDPointTreePoint>          m_aKDPoints;

	bool                                m_bGeneratingLowRes;
	tvector<float>                      m_aflLowResDrop;
	tvector<unsigned int>               m_aiLowResDrop;
};

class CTerrainLumpManager
{
	friend class CTerrainLump;

public:
										CTerrainLumpManager(class CPlanet* pPlanet);

public:
	void								AddLump(size_t iTerrain, const DoubleVector2D& vecLumpMin, const DoubleVector2D& vecLumpMax);
	void								RemoveLump(size_t iLump);

	CTerrainLump*						GetLump(size_t iLump) const;
	size_t								GetNumLumps() const { return m_apLumps.size(); }

	void								Think();
	void                                AddNearbyLumps();
	void								GenerateLump(size_t iLump);
	void								Render();

	bool                                FindApproximateElevation(const DoubleVector& vec3DLocal, float& flElevation) const;

	bool                                IsGenerating() const;

private:
	void								RemoveLumpNoLock(size_t iLump);

protected:
	class CPlanet*						m_pPlanet;
	tvector<CTerrainLump*>              m_apLumps;

	double								m_flNextLumpCheck;

	static class CParallelizer*         s_pLumpGenerator;
};
