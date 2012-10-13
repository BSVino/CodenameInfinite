#include "voxel_tree.h"

#include <tinker/cvar.h>

#include "entities/structures/spire.h"
#include "entities/sp_game.h"
#include "entities/sp_player.h"
#include "entities/sp_playercharacter.h"

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
	Vector vecBlockPoint = m_pSpire->GetLocalTransform().InvertedRT() * vecPlanetLocal;

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

	return m_pSpire->GetLocalTransform() * CScalableVector(x, y, z);
}

void CVoxelTree::RebuildChunkVBOs()
{
	for (auto it = m_aChunks.begin(); it != m_aChunks.end(); it++)
		it->second.BuildVBO();
}

void Rebuild(class CCommand* pCommand, tvector<tstring>& asTokens, const tstring& sCommand)
{
	CSPPlayer* pPlayer = SPGame()->GetLocalSPPlayer();
	if (!pPlayer)
		return;

	CPlayerCharacter* pCharacter = pPlayer->GetPlayerCharacter();

	CSpire* pSpire = pCharacter->GetNearestSpire();

	pSpire->GetVoxelTree()->RebuildChunkVBOs();
}

CCommand voxel_rebuild("voxel_rebuild", Rebuild);

