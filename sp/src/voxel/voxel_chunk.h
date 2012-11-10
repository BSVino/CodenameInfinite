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
	void       Render() const;

	bool       PlaceBlock(item_t eItem, const IVector& vecBlock);
	item_t     GetBlock(const IVector& vecBlock) const;
	item_t     RemoveBlock(const IVector& vecLocal);
	bool       PlaceDesignation(item_t eItem, const IVector& vecBlock);
	item_t     GetDesignation(const IVector& vecBlock) const;
	item_t     RemoveDesignation(const IVector& vecBlock);

	item_t     GetBlockGlobal(const IVector& vecBlock) const;
	item_t     GetDesignationGlobal(const IVector& vecBlock) const;

	const IVector   GetChunkCoordinates() const { return m_vecChunk; }

	void       BuildVBO();

private:
	CVoxelTree*  m_pTree;
	IVector      m_vecChunk;
	size_t       m_iVBO;
	size_t       m_iVBOSize;
	size_t       m_iVBODesignations;
	size_t       m_iVBODesignationsSize;

	unsigned char     m_aBlocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
	size_t            m_aPhysicsBlocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
	unsigned char     m_aDesignations[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
};
