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

	CVoxelChunk*    GetChunk(const IVector& vecChunk);
	CVoxelChunk*    GetChunkIfExists(const IVector& vecChunk);

	IVector         ToVoxelCoordinates(const CScalableVector& vecPlanetLocal) const;
	CScalableVector ToLocalCoordinates(const IVector& vecBlock) const;

	void            SetSpire(class CSpire* pSpire) { m_hSpire = pSpire; }
	class CSpire*   GetSpire() { return m_hSpire; }

	void       RebuildChunkVBOs();

private:
	CEntityHandle<CSpire>      m_hSpire;

	tmap<IVector, CVoxelChunk> m_aChunks;
};
