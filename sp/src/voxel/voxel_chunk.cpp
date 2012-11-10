#include "voxel_chunk.h"

#include <game/gameserver.h>
#include <renderer/game_renderer.h>
#include <renderer/renderingcontext.h>
#include <physics/physics.h>

#include "voxel_tree.h"
#include "entities/structures/spire.h"

CVoxelChunk::CVoxelChunk(CVoxelTree* pTree, const IVector& vecChunk)
{
	m_pTree = pTree;
	m_vecChunk = vecChunk;

	memset(m_aBlocks, 0, sizeof(m_aBlocks));
	memset(m_aDesignations, 0, sizeof(m_aDesignations));
	memset(m_aPhysicsBlocks, 0, sizeof(m_aPhysicsBlocks));

	m_iVBO = 0;
	m_iVBODesignations = 0;
}

CVoxelChunk::~CVoxelChunk()
{
	if (m_iVBO)
		GameServer()->GetRenderer()->UnloadVertexDataFromGL(m_iVBO);
	if (m_iVBODesignations)
		GameServer()->GetRenderer()->UnloadVertexDataFromGL(m_iVBODesignations);

	for (size_t x = 0; x < CHUNK_SIZE; x++)
	{
		for (size_t y = 0; y < CHUNK_SIZE; y++)
		{
			for (size_t z = 0; z < CHUNK_SIZE; z++)
			{
				if (m_aPhysicsBlocks[x][y][z])
					GamePhysics()->RemoveExtra(m_aPhysicsBlocks[x][y][z]);
			}
		}
	}
}

void CVoxelChunk::Render() const
{
	if (m_iVBO && !GameServer()->GetRenderer()->IsRenderingTransparent())
	{
		CRenderingContext c(GameServer()->GetRenderer(), true);

		c.ResetTransformations();
		c.Transform(m_pTree->GetSpire()->BaseGetRenderTransform());

		c.UseMaterial("textures/items1.mat");

		c.BeginRenderVertexArray(m_iVBO);
		c.SetPositionBuffer(0*sizeof(float), 5*sizeof(float));
		c.SetTexCoordBuffer(3*sizeof(float), 5*sizeof(float), 0);
		c.EndRenderVertexArray(m_iVBOSize);
	}
	else if (m_iVBODesignations && GameServer()->GetRenderer()->IsRenderingTransparent())
	{
		CRenderingContext c(GameServer()->GetRenderer(), true);

		c.ResetTransformations();
		c.Transform(m_pTree->GetSpire()->BaseGetRenderTransform());

		c.UseMaterial("textures/items1.mat");
		c.SetBlend(BLEND_ALPHA);
		c.SetUniform("vecColor", Vector4D(1, 1, 1, 0.4f));
		c.SetDepthMask(false);

		c.BeginRenderVertexArray(m_iVBODesignations);
		c.SetPositionBuffer(0*sizeof(float), 5*sizeof(float));
		c.SetTexCoordBuffer(3*sizeof(float), 5*sizeof(float), 0);
		c.EndRenderVertexArray(m_iVBODesignationsSize);
	}
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
	m_aDesignations[vecBlock.x][vecBlock.y][vecBlock.z] = ITEM_NONE;

	BuildVBO();

	return true;
}

item_t CVoxelChunk::GetBlock(const IVector& vecBlock) const
{
	TAssertNoMsg(vecBlock.x >= 0 && vecBlock.x < CHUNK_SIZE);
	TAssertNoMsg(vecBlock.y >= 0 && vecBlock.y < CHUNK_SIZE);
	TAssertNoMsg(vecBlock.z >= 0 && vecBlock.z < CHUNK_SIZE);

	return (item_t)m_aBlocks[vecBlock.x][vecBlock.y][vecBlock.z];
}

item_t CVoxelChunk::RemoveBlock(const IVector& vecBlock)
{
	TAssertNoMsg(vecBlock.x >= 0 && vecBlock.x < CHUNK_SIZE);
	TAssertNoMsg(vecBlock.y >= 0 && vecBlock.y < CHUNK_SIZE);
	TAssertNoMsg(vecBlock.z >= 0 && vecBlock.z < CHUNK_SIZE);

	TAssert(m_aBlocks[vecBlock.x][vecBlock.y][vecBlock.z]);
	if (!m_aBlocks[vecBlock.x][vecBlock.y][vecBlock.z])
		return ITEM_NONE;

	item_t eReturn = (item_t)m_aBlocks[vecBlock.x][vecBlock.y][vecBlock.z];
	m_aBlocks[vecBlock.x][vecBlock.y][vecBlock.z] = ITEM_NONE;

	BuildVBO();

	return eReturn;
}

bool CVoxelChunk::PlaceDesignation(item_t eItem, const IVector& vecBlock)
{
	TAssertNoMsg(vecBlock.x >= 0 && vecBlock.x < CHUNK_SIZE);
	TAssertNoMsg(vecBlock.y >= 0 && vecBlock.y < CHUNK_SIZE);
	TAssertNoMsg(vecBlock.z >= 0 && vecBlock.z < CHUNK_SIZE);

	if (m_aBlocks[vecBlock.x][vecBlock.y][vecBlock.z])
		return false;

	TAssertNoMsg(eItem < pow(2.0f, (int)sizeof(m_aBlocks[0][0][0])*8));

	m_aDesignations[vecBlock.x][vecBlock.y][vecBlock.z] = eItem;

	BuildVBO();

	return true;
}

item_t CVoxelChunk::GetDesignation(const IVector& vecBlock) const
{
	TAssertNoMsg(vecBlock.x >= 0 && vecBlock.x < CHUNK_SIZE);
	TAssertNoMsg(vecBlock.y >= 0 && vecBlock.y < CHUNK_SIZE);
	TAssertNoMsg(vecBlock.z >= 0 && vecBlock.z < CHUNK_SIZE);

	return (item_t)m_aDesignations[vecBlock.x][vecBlock.y][vecBlock.z];
}

item_t CVoxelChunk::RemoveDesignation(const IVector& vecBlock)
{
	TAssertNoMsg(vecBlock.x >= 0 && vecBlock.x < CHUNK_SIZE);
	TAssertNoMsg(vecBlock.y >= 0 && vecBlock.y < CHUNK_SIZE);
	TAssertNoMsg(vecBlock.z >= 0 && vecBlock.z < CHUNK_SIZE);

	TAssert(m_aDesignations[vecBlock.x][vecBlock.y][vecBlock.z]);
	if (!m_aDesignations[vecBlock.x][vecBlock.y][vecBlock.z])
		return ITEM_NONE;

	item_t eReturn = (item_t)m_aDesignations[vecBlock.x][vecBlock.y][vecBlock.z];
	m_aDesignations[vecBlock.x][vecBlock.y][vecBlock.z] = ITEM_NONE;

	BuildVBO();

	return eReturn;
}

item_t CVoxelChunk::GetBlockGlobal(const IVector& vecBlock) const
{
	IVector vecChunk = vecBlock.FindChunk();
	if (vecChunk == m_vecChunk)
		return GetBlock(vecBlock.FindChunkCoordinates());

	return m_pTree->GetBlock(vecBlock);
}

item_t CVoxelChunk::GetDesignationGlobal(const IVector& vecBlock) const
{
	IVector vecChunk = vecBlock.FindChunk();
	if (vecChunk == m_vecChunk)
		return GetDesignation(vecBlock.FindChunkCoordinates());

	return m_pTree->GetDesignation(vecBlock);
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
	if (m_iVBODesignations)
		GameServer()->GetRenderer()->UnloadVertexDataFromGL(m_iVBODesignations);

	tvector<float> aflVerts;
	size_t iVBOVerts = 0;

	tvector<float> aflDesignationVerts;
	size_t iVBODesignationVerts = 0;

	for (size_t x = 0; x < CHUNK_SIZE; x++)
	{
		for (size_t y = 0; y < CHUNK_SIZE; y++)
		{
			for (size_t z = 0; z < CHUNK_SIZE; z++)
			{
				if (m_aBlocks[x][y][z] && !m_aPhysicsBlocks[x][y][z])
				{
					Vector vecCubeCenter = Vector(0.5f, 0.5f, 0.5f) + (m_vecChunk*CHUNK_SIZE + IVector(x, y, z)).GetVoxelSpaceCoordinates();
					TVector vecCubeCenterPlanet = m_pTree->GetSpire()->GetLocalTransform() * vecCubeCenter;
					m_aPhysicsBlocks[x][y][z] = GamePhysics()->AddExtraCube(m_pTree->GetSpire()->GameData().TransformPointLocalToPhysics(vecCubeCenterPlanet.GetUnits(SCALE_METER)), 1);
				}
				else if (!m_aBlocks[x][y][z] && m_aPhysicsBlocks[x][y][z])
				{
					GamePhysics()->RemoveExtra(m_aPhysicsBlocks[x][y][z]);
					m_aPhysicsBlocks[x][y][z] = 0;
				}

				if (!m_aBlocks[x][y][z] && !m_aDesignations[x][y][z])
					continue;

				TAssert(!(m_aBlocks[x][y][z] && m_aDesignations[x][y][z]));

				tvector<float>& aflBuffer = (!!m_aBlocks[x][y][z]?aflVerts:aflDesignationVerts);

				IVector vecChunkOrigin = m_vecChunk*CHUNK_SIZE;
				Vector vxyz = (vecChunkOrigin + IVector(x, y, z)).GetVoxelSpaceCoordinates();
				Vector vxyZ = (vecChunkOrigin + IVector(x, y, z+1)).GetVoxelSpaceCoordinates();
				Vector vxYz = (vecChunkOrigin + IVector(x, y+1, z)).GetVoxelSpaceCoordinates();
				Vector vxYZ = (vecChunkOrigin + IVector(x, y+1, z+1)).GetVoxelSpaceCoordinates();
				Vector vXyz = (vecChunkOrigin + IVector(x+1, y, z)).GetVoxelSpaceCoordinates();
				Vector vXyZ = (vecChunkOrigin + IVector(x+1, y, z+1)).GetVoxelSpaceCoordinates();
				Vector vXYz = (vecChunkOrigin + IVector(x+1, y+1, z)).GetVoxelSpaceCoordinates();
				Vector vXYZ = (vecChunkOrigin + IVector(x+1, y+1, z+1)).GetVoxelSpaceCoordinates();

				bool bDrawSide;

				bDrawSide = !GetBlockGlobal(vecChunkOrigin + IVector(x-1, y, z));
				if (bDrawSide && m_aDesignations[x][y][z])
					bDrawSide = !GetDesignationGlobal(vecChunkOrigin + IVector(x-1, y, z));

				if (bDrawSide)
				{
					PushVert(aflBuffer, vxyz, Vector2D(0.0f, 0.75f));
					PushVert(aflBuffer, vxyZ, Vector2D(0.25f, 0.75f));
					PushVert(aflBuffer, vxYZ, Vector2D(0.25f, 1.0f));

					PushVert(aflBuffer, vxyz, Vector2D(0.0f, 0.75f));
					PushVert(aflBuffer, vxYZ, Vector2D(0.25f, 1.0f));
					PushVert(aflBuffer, vxYz, Vector2D(0.0f, 1.0f));

					if (m_aDesignations[x][y][z])
						iVBODesignationVerts += 6;
					else
						iVBOVerts += 6;
				}

				bDrawSide = !GetBlockGlobal(vecChunkOrigin + IVector(x+1, y, z));
				if (bDrawSide && m_aDesignations[x][y][z])
					bDrawSide = !GetDesignationGlobal(vecChunkOrigin + IVector(x+1, y, z));

				if (bDrawSide)
				{
					PushVert(aflBuffer, vXyZ, Vector2D(0.0f, 0.75f));
					PushVert(aflBuffer, vXyz, Vector2D(0.25f, 0.75f));
					PushVert(aflBuffer, vXYz, Vector2D(0.25f, 1.0f));

					PushVert(aflBuffer, vXyZ, Vector2D(0.0f, 0.75f));
					PushVert(aflBuffer, vXYz, Vector2D(0.25f, 1.0f));
					PushVert(aflBuffer, vXYZ, Vector2D(0.0f, 1.0f));

					if (m_aDesignations[x][y][z])
						iVBODesignationVerts += 6;
					else
						iVBOVerts += 6;
				}

				bDrawSide = !GetBlockGlobal(vecChunkOrigin + IVector(x, y-1, z));
				if (bDrawSide && m_aDesignations[x][y][z])
					bDrawSide = !GetDesignationGlobal(vecChunkOrigin + IVector(x, y-1, z));

				if (bDrawSide)
				{
					PushVert(aflBuffer, vXyz, Vector2D(0.0f, 0.75f));
					PushVert(aflBuffer, vXyZ, Vector2D(0.25f, 0.75f));
					PushVert(aflBuffer, vxyZ, Vector2D(0.25f, 1.0f));

					PushVert(aflBuffer, vXyz, Vector2D(0.0f, 0.75f));
					PushVert(aflBuffer, vxyZ, Vector2D(0.25f, 1.0f));
					PushVert(aflBuffer, vxyz, Vector2D(0.0f, 1.0f));

					if (m_aDesignations[x][y][z])
						iVBODesignationVerts += 6;
					else
						iVBOVerts += 6;
				}

				bDrawSide = !GetBlockGlobal(vecChunkOrigin + IVector(x, y+1, z));
				if (bDrawSide && m_aDesignations[x][y][z])
					bDrawSide = !GetDesignationGlobal(vecChunkOrigin + IVector(x, y+1, z));

				if (bDrawSide)
				{
					PushVert(aflBuffer, vxYz, Vector2D(0.0f, 0.75f));
					PushVert(aflBuffer, vxYZ, Vector2D(0.25f, 0.75f));
					PushVert(aflBuffer, vXYZ, Vector2D(0.25f, 1.0f));

					PushVert(aflBuffer, vxYz, Vector2D(0.0f, 0.75f));
					PushVert(aflBuffer, vXYZ, Vector2D(0.25f, 1.0f));
					PushVert(aflBuffer, vXYz, Vector2D(0.0f, 1.0f));

					if (m_aDesignations[x][y][z])
						iVBODesignationVerts += 6;
					else
						iVBOVerts += 6;
				}

				bDrawSide = !GetBlockGlobal(vecChunkOrigin + IVector(x, y, z-1));
				if (bDrawSide && m_aDesignations[x][y][z])
					bDrawSide = !GetDesignationGlobal(vecChunkOrigin + IVector(x, y, z-1));

				if (bDrawSide)
				{
					PushVert(aflBuffer, vXyz, Vector2D(0.0f, 0.75f));
					PushVert(aflBuffer, vxyz, Vector2D(0.25f, 0.75f));
					PushVert(aflBuffer, vxYz, Vector2D(0.25f, 1.0f));

					PushVert(aflBuffer, vXyz, Vector2D(0.0f, 0.75f));
					PushVert(aflBuffer, vxYz, Vector2D(0.25f, 1.0f));
					PushVert(aflBuffer, vXYz, Vector2D(0.0f, 1.0f));

					if (m_aDesignations[x][y][z])
						iVBODesignationVerts += 6;
					else
						iVBOVerts += 6;
				}

				bDrawSide = !GetBlockGlobal(vecChunkOrigin + IVector(x, y, z+1));
				if (bDrawSide && m_aDesignations[x][y][z])
					bDrawSide = !GetDesignationGlobal(vecChunkOrigin + IVector(x, y, z+1));

				if (bDrawSide)
				{
					PushVert(aflBuffer, vxyZ, Vector2D(0.0f, 0.75f));
					PushVert(aflBuffer, vXyZ, Vector2D(0.25f, 0.75f));
					PushVert(aflBuffer, vXYZ, Vector2D(0.25f, 1.0f));

					PushVert(aflBuffer, vxyZ, Vector2D(0.0f, 0.75f));
					PushVert(aflBuffer, vXYZ, Vector2D(0.25f, 1.0f));
					PushVert(aflBuffer, vxYZ, Vector2D(0.0f, 1.0f));

					if (m_aDesignations[x][y][z])
						iVBODesignationVerts += 6;
					else
						iVBOVerts += 6;
				}
			}
		}
	}

	if (iVBOVerts)
	{
		m_iVBO = GameServer()->GetRenderer()->LoadVertexDataIntoGL(aflVerts.size()*sizeof(float), aflVerts.data());
		m_iVBOSize = iVBOVerts;
	}
	else
	{
		m_iVBO = 0;
		m_iVBOSize = 0;
	}

	if (iVBODesignationVerts)
	{
		m_iVBODesignations = GameServer()->GetRenderer()->LoadVertexDataIntoGL(aflDesignationVerts.size()*sizeof(float), aflDesignationVerts.data());
		m_iVBODesignationsSize = iVBODesignationVerts;
	}
	else
	{
		m_iVBODesignations = 0;
		m_iVBODesignationsSize = 0;
	}
}
