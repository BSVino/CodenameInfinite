#include "voxel_chunk.h"

#include <game/gameserver.h>
#include <renderer/game_renderer.h>
#include <renderer/renderingcontext.h>

#include "voxel_tree.h"
#include "entities/structures/spire.h"

CVoxelChunk::CVoxelChunk(CVoxelTree* pTree, const IVector& vecChunk)
{
	m_pTree = pTree;
	m_vecChunk = vecChunk;

	memset(m_aBlocks, 0, sizeof(m_aBlocks));

	m_iVBO = 0;
}

void CVoxelChunk::Render() const
{
	if (!m_iVBO)
		return;

	CRenderingContext c(GameServer()->GetRenderer(), true);

	c.ResetTransformations();
	c.Transform(m_pTree->GetSpire()->BaseGetRenderTransform());

	c.UseMaterial("textures/items1.mat");

	c.BeginRenderVertexArray(m_iVBO);
	c.SetPositionBuffer(0*sizeof(float), 5*sizeof(float));
	c.SetTexCoordBuffer(3*sizeof(float), 5*sizeof(float), 0);
	c.EndRenderVertexArray(m_iVBOSize);
}

bool CVoxelChunk::PlaceBlock(item_t eItem, const IVector& vecBlock)
{
	TAssertNoMsg(vecBlock.x >= 0 && vecBlock.x < CHUNK_SIZE);
	TAssertNoMsg(vecBlock.y >= 0 && vecBlock.y < CHUNK_SIZE);
	TAssertNoMsg(vecBlock.z >= 0 && vecBlock.z < CHUNK_SIZE);

	if (m_aBlocks[vecBlock.x][vecBlock.y][vecBlock.z])
		return false;

	TAssertNoMsg(eItem < pow(2.0f, (int)sizeof(m_aBlocks[0][0][0])*8));

	m_aBlocks[vecBlock.x][vecBlock.y][vecBlock.z] = eItem;

	BuildVBO();

	return true;
}

void PushVert(tvector<float>& aflVerts, const Vector& v, const Vector2D& vt)
{
	aflVerts.push_back(v.x);
	aflVerts.push_back(v.y);
	aflVerts.push_back(v.z);
	aflVerts.push_back(vt.x);
	aflVerts.push_back(vt.y);
}

void CVoxelChunk::BuildVBO()
{
	if (m_iVBO)
		GameServer()->GetRenderer()->UnloadVertexDataFromGL(m_iVBO);

	CVoxelChunk* px = m_pTree->GetChunkIfExists(m_vecChunk + IVector(-1, 0, 0));
	CVoxelChunk* pX = m_pTree->GetChunkIfExists(m_vecChunk + IVector(1, 0, 0));
	CVoxelChunk* py = m_pTree->GetChunkIfExists(m_vecChunk + IVector(0, -1, 0));
	CVoxelChunk* pY = m_pTree->GetChunkIfExists(m_vecChunk + IVector(0, 1, 0));
	CVoxelChunk* pz = m_pTree->GetChunkIfExists(m_vecChunk + IVector(0, 0, -1));
	CVoxelChunk* pZ = m_pTree->GetChunkIfExists(m_vecChunk + IVector(0, 0, 1));

	tvector<float> aflVerts;
	size_t iVBOVerts = 0;

	for (size_t x = 0; x < CHUNK_SIZE; x++)
	{
		for (size_t y = 0; y < CHUNK_SIZE; y++)
		{
			for (size_t z = 0; z < CHUNK_SIZE; z++)
			{
				if (!m_aBlocks[x][y][z])
					continue;

				IVector vecChunkOrigin = m_vecChunk*16;
				Vector vxyz = (vecChunkOrigin + IVector(x, y, z)).GetVoxelSpaceCoordinates();
				Vector vxyZ = (vecChunkOrigin + IVector(x, y, z+1)).GetVoxelSpaceCoordinates();
				Vector vxYz = (vecChunkOrigin + IVector(x, y+1, z)).GetVoxelSpaceCoordinates();
				Vector vxYZ = (vecChunkOrigin + IVector(x, y+1, z+1)).GetVoxelSpaceCoordinates();
				Vector vXyz = (vecChunkOrigin + IVector(x+1, y, z)).GetVoxelSpaceCoordinates();
				Vector vXyZ = (vecChunkOrigin + IVector(x+1, y, z+1)).GetVoxelSpaceCoordinates();
				Vector vXYz = (vecChunkOrigin + IVector(x+1, y+1, z)).GetVoxelSpaceCoordinates();
				Vector vXYZ = (vecChunkOrigin + IVector(x+1, y+1, z+1)).GetVoxelSpaceCoordinates();

				if (x == 0 && px && !px->m_aBlocks[CHUNK_SIZE-1][y][z] || !m_aBlocks[x-1][y][z])
				{
					PushVert(aflVerts, vxyz, Vector2D(0.0f, 0.75f));
					PushVert(aflVerts, vxyZ, Vector2D(0.25f, 0.75f));
					PushVert(aflVerts, vxYZ, Vector2D(0.25f, 1.0f));

					PushVert(aflVerts, vxyz, Vector2D(0.0f, 0.75f));
					PushVert(aflVerts, vxYZ, Vector2D(0.25f, 1.0f));
					PushVert(aflVerts, vxYz, Vector2D(0.0f, 1.0f));

					iVBOVerts += 6;
				}

				if (x == CHUNK_SIZE-1 && pX && !pX->m_aBlocks[0][y][z] || !m_aBlocks[x+1][y][z])
				{
					PushVert(aflVerts, vXyZ, Vector2D(0.0f, 0.75f));
					PushVert(aflVerts, vXyz, Vector2D(0.25f, 0.75f));
					PushVert(aflVerts, vXYz, Vector2D(0.25f, 1.0f));

					PushVert(aflVerts, vXyZ, Vector2D(0.0f, 0.75f));
					PushVert(aflVerts, vXYz, Vector2D(0.25f, 1.0f));
					PushVert(aflVerts, vXYZ, Vector2D(0.0f, 1.0f));

					iVBOVerts += 6;
				}

				if (y == 0 && py && !py->m_aBlocks[x][CHUNK_SIZE-1][z] || !m_aBlocks[x][y-1][z])
				{
					PushVert(aflVerts, vXyz, Vector2D(0.0f, 0.75f));
					PushVert(aflVerts, vXyZ, Vector2D(0.25f, 0.75f));
					PushVert(aflVerts, vxyZ, Vector2D(0.25f, 1.0f));

					PushVert(aflVerts, vXyz, Vector2D(0.0f, 0.75f));
					PushVert(aflVerts, vxyZ, Vector2D(0.25f, 1.0f));
					PushVert(aflVerts, vxyz, Vector2D(0.0f, 1.0f));

					iVBOVerts += 6;
				}

				if (y == CHUNK_SIZE-1 && pY && !pY->m_aBlocks[x][0][z] || !m_aBlocks[x][y+1][z])
				{
					PushVert(aflVerts, vxYz, Vector2D(0.0f, 0.75f));
					PushVert(aflVerts, vxYZ, Vector2D(0.25f, 0.75f));
					PushVert(aflVerts, vXYZ, Vector2D(0.25f, 1.0f));

					PushVert(aflVerts, vxYz, Vector2D(0.0f, 0.75f));
					PushVert(aflVerts, vXYZ, Vector2D(0.25f, 1.0f));
					PushVert(aflVerts, vXYz, Vector2D(0.0f, 1.0f));

					iVBOVerts += 6;
				}

				if (z == 0 && pz && !pz->m_aBlocks[x][y][CHUNK_SIZE-1] || !m_aBlocks[x][y][z-1])
				{
					PushVert(aflVerts, vXyz, Vector2D(0.0f, 0.75f));
					PushVert(aflVerts, vxyz, Vector2D(0.25f, 0.75f));
					PushVert(aflVerts, vxYz, Vector2D(0.25f, 1.0f));

					PushVert(aflVerts, vXyz, Vector2D(0.0f, 0.75f));
					PushVert(aflVerts, vxYz, Vector2D(0.25f, 1.0f));
					PushVert(aflVerts, vXYz, Vector2D(0.0f, 1.0f));

					iVBOVerts += 6;
				}

				if (z == CHUNK_SIZE-1 && pZ && !pZ->m_aBlocks[x][y][0] || !m_aBlocks[x][y][z+1])
				{
					PushVert(aflVerts, vxyZ, Vector2D(0.0f, 0.75f));
					PushVert(aflVerts, vXyZ, Vector2D(0.25f, 0.75f));
					PushVert(aflVerts, vXYZ, Vector2D(0.25f, 1.0f));

					PushVert(aflVerts, vxyZ, Vector2D(0.0f, 0.75f));
					PushVert(aflVerts, vXYZ, Vector2D(0.25f, 1.0f));
					PushVert(aflVerts, vxYZ, Vector2D(0.0f, 1.0f));

					iVBOVerts += 6;
				}
			}
		}
	}

	m_iVBO = GameServer()->GetRenderer()->LoadVertexDataIntoGL(aflVerts.size()*sizeof(float), aflVerts.data());
	m_iVBOSize = iVBOVerts;
}
