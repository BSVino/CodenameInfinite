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

public:
	void       Render() const;

	bool       PlaceBlock(item_t eItem, const IVector& vecBlock);

	void       BuildVBO();

private:
	CVoxelTree*  m_pTree;
	IVector      m_vecChunk;
	size_t       m_iVBO;
	size_t       m_iVBOSize;

	unsigned char     m_aBlocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
	size_t            m_aPhysicsBlocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
};
