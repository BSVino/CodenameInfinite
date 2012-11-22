#include "lump_voxels.h"

#include <renderer/renderingcontext.h>
#include <tengine/renderer/game_renderer.h>

#include "entities/sp_game.h"
#include "entities/sp_playercharacter.h"
#include "sp_renderer.h"

#include "planet.h"
#include "planet_terrain.h"
#include "terrain_chunks.h"
#include "terrain_lumps.h"

CLumpVoxelManager::CLumpVoxelManager(CPlanet* pPlanet)
{
	m_pPlanet = pPlanet;
}

void CLumpVoxelManager::LoadChunk(class CTerrainChunk* pChunk)
{
	CVoxelTree* pVoxelTree = GetLumpVoxel(pChunk->GetTerrain(), pChunk->GetMin2D());

	TAssert(pVoxelTree);
	if (!pVoxelTree)
		return;

	pVoxelTree->Load();
}

void CLumpVoxelManager::UnloadChunk(class CTerrainChunk* pChunk)
{
	CVoxelTree* pVoxelTree = GetLumpVoxel(pChunk->GetTerrain(), pChunk->GetMin2D());

	TAssert(pVoxelTree);
	if (!pVoxelTree)
		return;

	pVoxelTree->Unload();
}

CVoxelTree* CLumpVoxelManager::GetLumpVoxel(size_t iTerrain, const DoubleVector2D& vecMin)
{
	CLumpAddress oLumpAddress;
	oLumpAddress.iTerrain = iTerrain;
	oLumpAddress.vecMin = vecMin;

	auto it = m_aLumpVoxels.find(oLumpAddress);

	if (it == m_aLumpVoxels.end())
		return nullptr;

	return &it->second;
}

const CVoxelTree* CLumpVoxelManager::GetLumpVoxel(size_t iTerrain, const DoubleVector2D& vecMin) const
{
	CLumpAddress oLumpAddress;
	oLumpAddress.iTerrain = iTerrain;
	oLumpAddress.vecMin = vecMin;

	const auto it = m_aLumpVoxels.find(oLumpAddress);

	if (it == m_aLumpVoxels.end())
		return nullptr;

	return &it->second;
}

CVoxelTree* CLumpVoxelManager::AddLumpVoxel(CTerrainLump* pLump)
{
	CLumpAddress oLumpAddress;
	oLumpAddress.iTerrain = pLump->GetTerrain();
	oLumpAddress.vecMin = pLump->GetMin2D();

	return &m_aLumpVoxels.insert(tpair<CLumpAddress, CVoxelTree>(oLumpAddress, CVoxelTree(this, pLump))).first->second;
}

