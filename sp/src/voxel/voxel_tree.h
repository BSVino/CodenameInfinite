#pragma once

#include <tmap.h>

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

	void            SetSpire(class CSpire* pSpire) { m_pSpire = pSpire; }
	class CSpire*   GetSpire() { return m_pSpire; }

	void       RebuildChunkVBOs();

private:
	class CSpire*      m_pSpire;

	tmap<IVector, CVoxelChunk> m_aChunks;
};
