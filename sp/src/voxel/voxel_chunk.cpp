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

	for (size_t i = 1; i < ITEM_BLOCKS_TOTAL; i++)
	{
		m_aiVBO[i] = 0;
		m_aiVBODesignations[i] = 0;
	}
}

CVoxelChunk::~CVoxelChunk()
{
	for (size_t i = 1; i < ITEM_BLOCKS_TOTAL; i++)
	{
		if (m_aiVBO[i])
			GameServer()->GetRenderer()->UnloadVertexDataFromGL(m_aiVBO[i]);
		if (m_aiVBODesignations[i])
			GameServer()->GetRenderer()->UnloadVertexDataFromGL(m_aiVBODesignations[i]);
	}

	for (size_t i = 0; i < m_aiPhysicsEnts.size(); i++)
		GamePhysics()->RemoveExtra(m_aiPhysicsEnts[i]);
}

void CVoxelChunk::Render() const
{
	for (size_t i = 1; i < ITEM_BLOCKS_TOTAL; i++)
	{
		if (m_aiVBO[i] && !GameServer()->GetRenderer()->IsRenderingTransparent())
		{
			CRenderingContext c(GameServer()->GetRenderer(), true);

			c.ResetTransformations();
			c.Transform(m_pTree->GetSpire()->BaseGetRenderTransform());

			c.UseMaterial(GetItemMaterial((item_t)i));

			c.BeginRenderVertexArray(m_aiVBO[i]);
			c.SetPositionBuffer(0*sizeof(float), 5*sizeof(float));
			c.SetTexCoordBuffer(3*sizeof(float), 5*sizeof(float), 0);
			c.EndRenderVertexArray(m_aiVBOSize[i]);
		}
		else if (m_aiVBODesignations[i] && GameServer()->GetRenderer()->IsRenderingTransparent())
		{
			CRenderingContext c(GameServer()->GetRenderer(), true);

			c.ResetTransformations();
			c.Transform(m_pTree->GetSpire()->BaseGetRenderTransform());

			c.UseMaterial(GetItemMaterial((item_t)i));
			c.SetBlend(BLEND_ALPHA);
			c.SetUniform("vecColor", Vector4D(1, 1, 1, 0.4f));
			c.SetDepthMask(false);

			c.BeginRenderVertexArray(m_aiVBODesignations[i]);
			c.SetPositionBuffer(0*sizeof(float), 5*sizeof(float));
			c.SetTexCoordBuffer(3*sizeof(float), 5*sizeof(float), 0);
			c.EndRenderVertexArray(m_aiVBODesignationsSize[ITEM_BLOCKS_TOTAL]);
		}
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
	for (size_t i = 1; i < ITEM_BLOCKS_TOTAL; i++)
	{
		if (m_aiVBO[i])
			GameServer()->GetRenderer()->UnloadVertexDataFromGL(m_aiVBO[i]);
		if (m_aiVBODesignations[i])
			GameServer()->GetRenderer()->UnloadVertexDataFromGL(m_aiVBODesignations[i]);
	}

	tvector<tvector<float>> aaflVerts;
	tvector<size_t> aiVBOVerts;

	tvector<tvector<float>> aaflDesignationVerts;
	tvector<size_t> aiVBODesignationVerts;

	aaflVerts.resize(ITEM_BLOCKS_TOTAL);
	aiVBOVerts.resize(ITEM_BLOCKS_TOTAL);
	aaflDesignationVerts.resize(ITEM_BLOCKS_TOTAL);
	aiVBODesignationVerts.resize(ITEM_BLOCKS_TOTAL);

	bool bPhysicsDirty = false;

	for (size_t x = 0; x < CHUNK_SIZE; x++)
	{
		for (size_t y = 0; y < CHUNK_SIZE; y++)
		{
			for (size_t z = 0; z < CHUNK_SIZE; z++)
			{
				if (m_aBlocks[x][y][z] && !m_aPhysicsBlocks[x][y][z])
					bPhysicsDirty = true;
				else if (!m_aBlocks[x][y][z] && m_aPhysicsBlocks[x][y][z])
					bPhysicsDirty = true;

				if (!m_aBlocks[x][y][z] && !m_aDesignations[x][y][z])
					continue;

				TAssert(!(m_aBlocks[x][y][z] && m_aDesignations[x][y][z]));

				tvector<float>& aflBuffer = (!!m_aBlocks[x][y][z]?aaflVerts[m_aBlocks[x][y][z]]:aaflDesignationVerts[m_aBlocks[x][y][z]]);

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
						aiVBODesignationVerts[m_aBlocks[x][y][z]] += 6;
					else
						aiVBOVerts[m_aBlocks[x][y][z]] += 6;
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
						aiVBODesignationVerts[m_aBlocks[x][y][z]] += 6;
					else
						aiVBOVerts[m_aBlocks[x][y][z]] += 6;
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
						aiVBODesignationVerts[m_aBlocks[x][y][z]] += 6;
					else
						aiVBOVerts[m_aBlocks[x][y][z]] += 6;
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
						aiVBODesignationVerts[m_aBlocks[x][y][z]] += 6;
					else
						aiVBOVerts[m_aBlocks[x][y][z]] += 6;
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
						aiVBODesignationVerts[m_aBlocks[x][y][z]] += 6;
					else
						aiVBOVerts[m_aBlocks[x][y][z]] += 6;
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
						aiVBODesignationVerts[m_aBlocks[x][y][z]] += 6;
					else
						aiVBOVerts[m_aBlocks[x][y][z]] += 6;
				}
			}
		}
	}

	for (size_t i = 1; i < ITEM_BLOCKS_TOTAL; i++)
	{
		if (aiVBOVerts[i])
		{
			m_aiVBO[i] = GameServer()->GetRenderer()->LoadVertexDataIntoGL(aaflVerts[i].size()*sizeof(float), aaflVerts[i].data());
			m_aiVBOSize[i] = aiVBOVerts[i];
		}
		else
		{
			m_aiVBO[i] = 0;
			m_aiVBOSize[i] = 0;
		}

		if (aiVBODesignationVerts[i])
		{
			m_aiVBODesignations[i] = GameServer()->GetRenderer()->LoadVertexDataIntoGL(aaflDesignationVerts[i].size()*sizeof(float), aaflDesignationVerts[i].data());
			m_aiVBODesignationsSize[i] = aiVBODesignationVerts[i];
		}
		else
		{
			m_aiVBODesignations[i] = 0;
			m_aiVBODesignationsSize[i] = 0;
		}
	}

	if (bPhysicsDirty)
	{
		// Clear out all physics blocks.
		for (size_t i = 0; i < m_aiPhysicsEnts.size(); i++)
			GamePhysics()->RemoveExtra(m_aiPhysicsEnts[i]);

		m_aiPhysicsEnts.clear();

		memset(m_aPhysicsBlocks, 0, sizeof(m_aPhysicsBlocks));

		for (size_t x = 0; x < CHUNK_SIZE; x++)
		{
			for (size_t y = 0; y < CHUNK_SIZE; y++)
			{
				for (size_t z = 0; z < CHUNK_SIZE; z++)
				{
					if (!m_aBlocks[x][y][z])
						continue;

					if (m_aPhysicsBlocks[x][y][z])
						continue;

					// Find the largest continuous block area available to fill with a physics box
					IVector vecBottom(x, y, z);
					IVector vecSize(1, 1, 1);

					size_t iCurrent = x;
					while (m_aBlocks[++iCurrent][y][z])
					{
						if (iCurrent >= CHUNK_SIZE)
							break;
					}

					vecSize.x = iCurrent-x;

					iCurrent = y;
					do
					{
						iCurrent++;

						if (iCurrent >= CHUNK_SIZE)
							break;

						bool bCanExpand = true;
						for (size_t x1 = x; x1 < x+(size_t)vecSize.x; x1++)
						{
							if (!m_aBlocks[x1][iCurrent][z])
							{
								bCanExpand = false;
								break;
							}
						}

						if (!bCanExpand)
							break;
					}
					while (true);

					vecSize.y = iCurrent-y;

					iCurrent = z;
					do
					{
						iCurrent++;

						if (iCurrent >= CHUNK_SIZE)
							break;

						bool bCanExpand = true;
						for (size_t x1 = x; x1 < x+(size_t)vecSize.x; x1++)
						{
							for (size_t y1 = y; y1 < y+(size_t)vecSize.y; y1++)
							{
								if (!m_aBlocks[x1][y1][iCurrent])
								{
									bCanExpand = false;
									break;
								}
							}

							if (!bCanExpand)
								break;
						}

						if (!bCanExpand)
							break;
					}
					while (true);

					vecSize.z = iCurrent-z;

					Vector vecCubeCenter = (m_vecChunk*CHUNK_SIZE + vecBottom ).GetVoxelSpaceCoordinates() + vecSize.GetVoxelSpaceCoordinates()/2;
					TVector vecCubeCenterPlanet = m_pTree->GetSpire()->GetLocalTransform() * vecCubeCenter;
					size_t iPhysicsBox = GamePhysics()->AddExtraBox(m_pTree->GetSpire()->GameData().TransformPointLocalToPhysics(vecCubeCenterPlanet.GetMeters()), vecSize.GetVoxelSpaceCoordinates());
					m_aiPhysicsEnts.push_back(iPhysicsBox);

					for (int x1 = vecBottom.x; x1 < vecBottom.x+vecSize.x; x1++)
					{
						for (int y1 = vecBottom.y; y1 < vecBottom.y+vecSize.y; y1++)
						{
							for (int z1 = vecBottom.z; z1 < vecBottom.z+vecSize.z; z1++)
								m_aPhysicsBlocks[x1][y1][z1] = iPhysicsBox;
						}
					}
				}
			}
		}
	}
}
