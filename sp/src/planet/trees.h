#pragma once

#include <vector.h>
#include <tvector.h>
#include <tmap.h>

class CChunkTrees
{
	friend class CTreeManager;

public:
	CChunkTrees(class CTreeManager* pManager)
	{
		m_pManager = pManager;

		m_bTreesGenerated = false;
	}

public:
	void          GenerateTrees(class CTerrainChunk* pChunk);

	void          Load();
	void          Unload();

	void          Render() const;

private:
	class CTreeManager*   m_pManager;

	bool    m_bTreesGenerated;

	size_t          m_iTerrain;
	DoubleVector2D  m_vecMin;
	DoubleVector2D  m_vecMax;

	DoubleVector    m_vecLocalCenter;	// Chunk center in planet space
	double          m_flRadius;

	tvector<DoubleVector> m_avecOrigins;
	tvector<size_t>       m_aiPhysicsBoxes;
};

class CTreeManager
{
	friend class CChunkTrees;

public:
	CTreeManager(class CPlanet* pPlanet);

public:
	void          GenerateTrees(class CTerrainChunk* pChunk);

	void          LoadChunk(class CTerrainChunk* pChunk);
	void          UnloadChunk(class CTerrainChunk* pChunk);

	void          Render() const;

	CChunkTrees*  GetChunkTrees(size_t iTerrain, const DoubleVector2D& vecMin);
	CChunkTrees*  AddChunkTrees(size_t iTerrain, const DoubleVector2D& vecMin);

private:
	class CPlanet*  m_pPlanet;

	class CChunkAddress
	{
	public:
		size_t         iTerrain;
		DoubleVector2D vecMin;

	public:
		bool operator<(const CChunkAddress& b) const
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

	tmap<CChunkAddress, CChunkTrees> m_aChunkTrees;
};