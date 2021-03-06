#pragma once

#include "sp_common.h"
#include "entities/items/items.h"
#include "ivector.h"

#define CHUNK_SIZE 16

class CVoxelTree;

class CVoxelChunk
{
public:
	CVoxelChunk(CVoxelTree* pTree, const IVector& vecChunk);
	~CVoxelChunk();

public:
	void       Load();
	void       Unload();
	void       UnloadVBO();
	void       UnloadPhysics();

	void       Render(bool bTransparent) const;

	bool       PlaceBlock(item_t eItem, const IVector& vecBlock);
	item_t     GetBlock(const IVector& vecBlock) const;
	item_t     RemoveBlock(const IVector& vecLocal);
	bool       PlaceDesignation(item_t eItem, const IVector& vecBlock);
	item_t     GetDesignation(const IVector& vecBlock) const;
	item_t     RemoveDesignation(const IVector& vecBlock);

	item_t     GetBlockGlobal(const IVector& vecBlock) const;
	item_t     GetDesignationGlobal(const IVector& vecBlock) const;

	const IVector   GetChunkCoordinates() const { return m_vecChunk; }

private:
	CVoxelTree*  m_pTree;
	IVector      m_vecChunk;
	size_t       m_aiVBO[ITEM_BLOCKS_TOTAL];
	size_t       m_aiVBOSize[ITEM_BLOCKS_TOTAL];
	size_t       m_aiVBODesignations[ITEM_BLOCKS_TOTAL];
	size_t       m_aiVBODesignationsSize[ITEM_BLOCKS_TOTAL];
	tvector<int> m_aiPhysicsEnts;

	unsigned char     m_aBlocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
	size_t            m_aPhysicsBlocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
	unsigned char     m_aDesignations[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
};
