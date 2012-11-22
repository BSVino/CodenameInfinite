#pragma once

#include <tmap.h>

#include <tengine/game/entities/baseentity.h>

#include "sp_common.h"
#include "entities/items/items.h"
#include "voxel_chunk.h"
#include "ivector.h"

class CVoxelTree
{
	friend class CVoxelChunk;

public:
	CVoxelTree(class CLumpVoxelManager* pManager, class CTerrainLump* pLump);

public:
	void       Load();
	void       Unload();

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
	void            AddBlockDesignation(item_t eType, const IVector& vecMin, const IVector& vecMax);

	CVoxelChunk*    GetChunk(const IVector& vecChunk);
	const CVoxelChunk* GetChunkIfExists(const IVector& vecChunk) const;
	CVoxelChunk*    GetChunkIfExists(const IVector& vecChunk);

	IVector         ToVoxelCoordinates(const CScalableVector& vecPlanetLocal) const;
	CScalableVector ToLocalCoordinates(const IVector& vecBlock) const;

	class CTerrainLump*   GetLump() const;

	void                      CheckForLumpTransforms() const;
	const DoubleMatrix4x4&    GetPlanetToTree() const;
	const DoubleMatrix4x4&    GetTreeToPlanet() const;

	// Debug stuffs
	void       RebuildChunkVBOs();
	void       ClearChunks();

private:
	class CLumpVoxelManager*   m_pManager;

	size_t                     m_iLumpTerrain;
	DoubleVector2D             m_vecLumpMin;

	tmap<IVector, CVoxelChunk> m_aChunks;

	mutable bool               m_bTransformsInitialized;
	mutable DoubleMatrix4x4    m_mPlanetToTree;
	mutable DoubleMatrix4x4    m_mTreeToPlanet;
};
