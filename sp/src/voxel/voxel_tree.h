#pragma once

#include <tmap.h>

#include <tengine/game/entities/baseentity.h>

#include "sp_common.h"
#include "entities/items/items.h"
#include "voxel_chunk.h"
#include "ivector.h"

class CVoxelTree
{
public:
	void       Render() const;

	bool       PlaceBlock(item_t eItem, const CScalableVector& vecLocal);
	bool       PlaceBlock(item_t eItem, const IVector& vecBlock);
	item_t     GetBlock(const CScalableVector& vecLocal) const;
	item_t     GetBlock(const IVector& vecBlock) const;
	item_t     RemoveBlock(const IVector& vecBlock);
	bool       PlaceDesignation(item_t eItem, const IVector& vecBlock);
	item_t     GetDesignation(const IVector& vecBlock) const;
	item_t     RemoveDesignation(const IVector& vecBlock);

	const IVector   FindNearbyDesignation(const CScalableVector& vecLocal) const;

	CVoxelChunk*    GetChunk(const IVector& vecChunk);
	const CVoxelChunk* GetChunkIfExists(const IVector& vecChunk) const;
	CVoxelChunk*    GetChunkIfExists(const IVector& vecChunk);

	IVector         ToVoxelCoordinates(const CScalableVector& vecPlanetLocal) const;
	CScalableVector ToLocalCoordinates(const IVector& vecBlock) const;

	void            SetSpire(class CSpire* pSpire) { m_hSpire = pSpire; }
	class CSpire*   GetSpire() const { return m_hSpire; }

	// Debug stuffs
	void       RebuildChunkVBOs();
	void       ClearChunks();

private:
	CEntityHandle<CSpire>      m_hSpire;

	tmap<IVector, CVoxelChunk> m_aChunks;
};
