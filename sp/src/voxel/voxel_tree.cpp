#include "voxel_tree.h"

#include <tinker/cvar.h>

#include "entities/sp_game.h"
#include "entities/sp_player.h"
#include "entities/sp_playercharacter.h"
#include "entities/items/pickup.h"
#include "planet/terrain_lumps.h"
#include "planet/lump_voxels.h"
#include "planet/planet.h"
#include "planet/terrain_chunks.h"

CVoxelTree::CVoxelTree(CLumpVoxelManager* pManager, CTerrainLump* pLump)
{
	m_pManager = pManager;
	m_iLumpTerrain = pLump->GetTerrain();
	m_vecLumpMin = pLump->GetMin2D();

	m_bTransformsInitialized = false;
}

void CVoxelTree::Load()
{
	for (auto it = m_aChunks.begin(); it != m_aChunks.end(); it++)
		it->second.Load();
}

void CVoxelTree::Unload()
{
	for (auto it = m_aChunks.begin(); it != m_aChunks.end(); it++)
		it->second.Unload();
}

void CVoxelTree::Render() const
{
	for (auto it = m_aChunks.begin(); it != m_aChunks.end(); it++)
		it->second.Render();
}

bool CVoxelTree::PlaceBlock(item_t eItem, const CScalableVector& vecLocal)
{
	IVector vecBlock = ToVoxelCoordinates(vecLocal);
	IVector vecChunk = vecBlock.FindChunk();
	IVector vecChunkCoordinates = vecBlock.FindChunkCoordinates();

	return GetChunk(vecChunk)->PlaceBlock(eItem, vecChunkCoordinates);
}

bool CVoxelTree::PlaceBlock(item_t eItem, const IVector& vecBlock)
{
	IVector vecChunk = vecBlock.FindChunk();
	IVector vecChunkCoordinates = vecBlock.FindChunkCoordinates();

	return GetChunk(vecChunk)->PlaceBlock(eItem, vecChunkCoordinates);
}

item_t CVoxelTree::GetBlock(const CScalableVector& vecLocal) const
{
	IVector vecBlock = ToVoxelCoordinates(vecLocal);
	IVector vecChunk = vecBlock.FindChunk();
	IVector vecChunkCoordinates = vecBlock.FindChunkCoordinates();

	const CVoxelChunk* pChunk = GetChunkIfExists(vecChunk);
	if (pChunk)
		return pChunk->GetBlock(vecChunkCoordinates);

	return ITEM_NONE;
}

item_t CVoxelTree::GetBlock(const IVector& vecBlock) const
{
	IVector vecChunk = vecBlock.FindChunk();
	IVector vecChunkCoordinates = vecBlock.FindChunkCoordinates();

	const CVoxelChunk* pChunk = GetChunkIfExists(vecChunk);
	if (pChunk)
		return pChunk->GetBlock(vecChunkCoordinates);

	return ITEM_NONE;
}

item_t CVoxelTree::RemoveBlock(const IVector& vecBlock)
{
	IVector vecChunk = vecBlock.FindChunk();
	IVector vecChunkCoordinates = vecBlock.FindChunkCoordinates();

	item_t eItem = GetChunk(vecChunk)->RemoveBlock(vecChunkCoordinates);

	TAssert(GetLump());
	if (!GetLump())
		return eItem;

	CPickup* pPickup = GameServer()->Create<CPickup>("CPickup");
	pPickup->GameData().SetPlanet(GetLump()->GetPlanet());
	pPickup->SetGlobalTransform(GetLump()->GetPlanet()->GetGlobalTransform()); // Avoid floating point precision problems
	pPickup->SetMoveParent(GetLump()->GetPlanet());
	pPickup->SetLocalOrigin(ToLocalCoordinates(vecBlock) + (Vector(0.5f, 0.5f, 0.5f) + GetLump()->GetLocalCenter().Normalized()));
	pPickup->SetItem(eItem);

	return eItem;
}

bool CVoxelTree::PlaceDesignation(item_t eItem, const IVector& vecBlock)
{
	IVector vecChunk = vecBlock.FindChunk();
	IVector vecChunkCoordinates = vecBlock.FindChunkCoordinates();

	return GetChunk(vecChunk)->PlaceDesignation(eItem, vecChunkCoordinates);
}

item_t CVoxelTree::GetDesignation(const IVector& vecBlock) const
{
	IVector vecChunk = vecBlock.FindChunk();
	IVector vecChunkCoordinates = vecBlock.FindChunkCoordinates();

	const CVoxelChunk* pChunk = GetChunkIfExists(vecChunk);
	if (pChunk)
		return pChunk->GetDesignation(vecChunkCoordinates);

	return ITEM_NONE;
}

item_t CVoxelTree::RemoveDesignation(const IVector& vecBlock)
{
	IVector vecChunk = vecBlock.FindChunk();
	IVector vecChunkCoordinates = vecBlock.FindChunkCoordinates();

	item_t eItem = GetChunk(vecChunk)->RemoveDesignation(vecChunkCoordinates);

	return eItem;
}

class CVoxelChunkArea
{
public:
	IVector    m_vecChunk;
	float      m_flDistance;
};

inline bool ChunkDistanceCompare(const CVoxelChunkArea& l, const CVoxelChunkArea& r)
{
	return l.m_flDistance < r.m_flDistance;
}

const IVector CVoxelTree::FindNearbyDesignation(const CScalableVector& vecLocal) const
{
	tvector<CVoxelChunkArea> aChunks;
	aChunks.reserve(m_aChunks.size());

	for (auto it = m_aChunks.begin(); it != m_aChunks.end(); it++)
	{
		CVoxelChunkArea& oArea = aChunks.push_back();
		oArea.m_vecChunk = it->second.GetChunkCoordinates();

		TVector vecChunkCenter = ToLocalCoordinates(oArea.m_vecChunk*CHUNK_SIZE + IVector(CHUNK_SIZE/2, CHUNK_SIZE/2, CHUNK_SIZE/2));
		oArea.m_flDistance = (float)(vecLocal-vecChunkCenter).GetMeters().LengthSqr();

		push_heap(aChunks.begin(), aChunks.end(), ChunkDistanceCompare);
	}

	sort_heap(aChunks.begin(), aChunks.end(), ChunkDistanceCompare);

	for (size_t i = 0; i < aChunks.size(); i++)
	{
		const CVoxelChunk* pChunk = GetChunkIfExists(aChunks[i].m_vecChunk);
		TAssert(pChunk);
		if (!pChunk)
			continue;

		for (int x = 0; x < CHUNK_SIZE; x++)
		{
			for (int y = 0; y < CHUNK_SIZE; y++)
			{
				for (int z = 0; z < CHUNK_SIZE; z++)
				{
					item_t eDesignation = pChunk->GetDesignation(IVector(x, y, z));
					if (eDesignation)
						return pChunk->GetChunkCoordinates()*CHUNK_SIZE + IVector(x, y, z);
				}
			}
		}
	}

	return IVector(0, 0, 0);
}

void CVoxelTree::AddBlockDesignation(item_t eType, const IVector& vecMin, const IVector& vecMax)
{
	TAssert(m_pManager->GetPlanet()->GetChunkManager()->HasGroupCenter());
	if (!m_pManager->GetPlanet()->GetChunkManager()->HasGroupCenter())
		return;

	DoubleMatrix4x4 mLocalToPhysics = m_pManager->GetPlanet()->GetChunkManager()->GetPlanetToGroupCenterTransform();
	DoubleMatrix4x4 mPhysicsToLocal = m_pManager->GetPlanet()->GetChunkManager()->GetGroupCenterToPlanetTransform();

	TAssert(vecMin.x <= vecMax.x);
	TAssert(vecMin.y <= vecMax.y);
	TAssert(vecMin.z <= vecMax.z);

	for (int x = 0; x <= vecMax.x-vecMin.x; x++)
	{
		for (int z = 0; z <= vecMax.z-vecMin.z; z++)
		{
			// Do a traceline from down to up. The first spot that it hits is the lowest we can designate any blocks.
			// Below that is just empty space!

			TVector vecColumn = ToLocalCoordinates(vecMin + IVector(x, 0, z)) + Vector(0.5f, 0.5f, 0.5f);
			Vector vecColumnPhysics = mLocalToPhysics * vecColumn;

			CTraceResult tr;
			GamePhysics()->TraceLine(tr, vecColumnPhysics - Vector(0, 1000, 0), vecColumnPhysics + Vector(0, 1000, 0));

			TAssert(tr.m_flFraction < 1);
			if (tr.m_flFraction == 1)
				continue;

			TVector vecHit = TVector(mPhysicsToLocal * tr.m_vecHit);
			int iLowest = ToVoxelCoordinates(vecHit).y;

			iLowest -= vecMin.y;
			if (iLowest < 0)
				iLowest = 0;

			for (int y = iLowest; y <= vecMax.y-vecMin.y; y++)
				PlaceDesignation(eType, vecMin + IVector(x, y, z));
		}
	}
}

CVoxelChunk* CVoxelTree::GetChunk(const IVector& vecChunk)
{
	auto it = m_aChunks.find(vecChunk);
	if (it == m_aChunks.end())
	{
		m_aChunks.insert(tpair<IVector, CVoxelChunk>(vecChunk, CVoxelChunk(this, vecChunk)));
		return &m_aChunks.find(vecChunk)->second;
	}
	else
		return &it->second;
}

const CVoxelChunk* CVoxelTree::GetChunkIfExists(const IVector& vecChunk) const
{
	auto it = m_aChunks.find(vecChunk);
	if (it == m_aChunks.end())
		return nullptr;
	else
		return &it->second;
}

CVoxelChunk* CVoxelTree::GetChunkIfExists(const IVector& vecChunk)
{
	auto it = m_aChunks.find(vecChunk);
	if (it == m_aChunks.end())
		return nullptr;
	else
		return &it->second;
}

IVector CVoxelTree::ToVoxelCoordinates(const CScalableVector& vecPlanetLocal) const
{
	Vector vecBlockPoint = GetPlanetToTree() * vecPlanetLocal;

	IVector vecVoxel;

	if (vecBlockPoint.x > 0)
		vecVoxel.x = (int)(vecBlockPoint.x - fmod(vecBlockPoint.x, 1.0f));
	else
		vecVoxel.x = (int)(vecBlockPoint.x - (1+fmod(vecBlockPoint.x, 1.0f)));

	if (vecBlockPoint.y > 0)
		vecVoxel.y = (int)(vecBlockPoint.y - fmod(vecBlockPoint.y, 1.0f));
	else
		vecVoxel.y = (int)(vecBlockPoint.y - (1+fmod(vecBlockPoint.y, 1.0f)));

	if (vecBlockPoint.z > 0)
		vecVoxel.z = (int)(vecBlockPoint.z - fmod(vecBlockPoint.z, 1.0f));
	else
		vecVoxel.z = (int)(vecBlockPoint.z - (1+fmod(vecBlockPoint.z, 1.0f)));

	return vecVoxel;
}

CScalableVector CVoxelTree::ToLocalCoordinates(const IVector& vecBlock) const
{
	CScalableFloat x((float)vecBlock.x);
	CScalableFloat y((float)vecBlock.y);
	CScalableFloat z((float)vecBlock.z);

	return GetTreeToPlanet() * CScalableVector(x, y, z);
}

void CVoxelTree::CheckForLumpTransforms() const
{
	if (m_bTransformsInitialized)
		return;

	if (!GetLump())
		return;

	if (GetLump()->GetLumpToPlanet() == DoubleMatrix4x4())
		return;

	if (GetLump()->GetPlanetToLump() == DoubleMatrix4x4())
		return;

	m_bTransformsInitialized = true;

	m_mTreeToPlanet = GetLump()->GetLumpToPlanet();
	m_mPlanetToTree = GetLump()->GetPlanetToLump();
}

const DoubleMatrix4x4& CVoxelTree::GetPlanetToTree() const
{
	CheckForLumpTransforms();

	TAssert(m_bTransformsInitialized);

	return m_mPlanetToTree;
}

const DoubleMatrix4x4& CVoxelTree::GetTreeToPlanet() const
{
	CheckForLumpTransforms();

	TAssert(m_bTransformsInitialized);

	return m_mTreeToPlanet;
}

CTerrainLump* CVoxelTree::GetLump() const
{
	CLumpAddress oLump;
	oLump.iTerrain = m_iLumpTerrain;
	oLump.vecMin = m_vecLumpMin;

	return m_pManager->GetPlanet()->GetLumpManager()->GetLump(oLump);
}

void CVoxelTree::RebuildChunkVBOs()
{
	for (auto it = m_aChunks.begin(); it != m_aChunks.end(); it++)
		it->second.Load();
}

void CVoxelTree::ClearChunks()
{
	m_aChunks.clear();
}

void VoxelRebuild(class CCommand* pCommand, tvector<tstring>& asTokens, const tstring& sCommand)
{
	CSPPlayer* pPlayer = SPGame()->GetLocalSPPlayer();
	if (!pPlayer)
		return;

	CPlayerCharacter* pCharacter = pPlayer->GetPlayerCharacter();

	TUnimplemented();
	/*CSpire* pSpire = pCharacter->GetNearestSpire();

	pSpire->GetVoxelTree()->RebuildChunkVBOs();*/
}

CCommand voxel_rebuild("voxel_rebuild", VoxelRebuild);

void VoxelClear(class CCommand* pCommand, tvector<tstring>& asTokens, const tstring& sCommand)
{
	CSPPlayer* pPlayer = SPGame()->GetLocalSPPlayer();
	if (!pPlayer)
		return;

	CPlayerCharacter* pCharacter = pPlayer->GetPlayerCharacter();

	TUnimplemented();

	/*CSpire* pSpire = pCharacter->GetNearestSpire();

	pSpire->GetVoxelTree()->ClearChunks();*/
}

CCommand voxel_clear("voxel_clear", VoxelClear);

