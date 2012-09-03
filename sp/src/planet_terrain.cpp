#include "planet_terrain.h"

#include <geometry.h>
#include <mtrand.h>
#include <parallelize.h>

#include <renderer/renderingcontext.h>
#include <tinker/cvar.h>
#include <tinker/application.h>
#include <tinker/profiler.h>
#include <tengine/renderer/game_renderer.h>

#include "sp_game.h"
#include "sp_renderer.h"
#include "sp_playercharacter.h"
#include "planet.h"
#include "terrain_chunks.h"

CPlanetTerrain::CPlanetTerrain(class CPlanet* pPlanet, Vector vecDirection)
{
	m_pPlanet = pPlanet;
	m_vecDirection = vecDirection;

	m_iShell1VBO = 0;
	m_iShell2VBO = 0;

	m_bGeneratingShell2 = false;
}

void CPlanetTerrain::Think()
{
	if (!m_iShell1VBO)
		CreateShell1VBO();

	if (m_bGeneratingShell2)
	{
		m_pPlanet->s_pShell2Generator->LockData();
		if (m_aflShell2Drop.size())
		{
			m_iShell2VBO = CRenderer::LoadVertexDataIntoGL(m_aflShell2Drop.size() * sizeof(float), m_aflShell2Drop.data());
			m_iShell2IBO = CRenderer::LoadIndexDataIntoGL(m_aiShell2Drop.size() * sizeof(unsigned int), m_aiShell2Drop.data());
			m_bGeneratingShell2 = false;
			m_aflShell2Drop.set_capacity(0);
			m_aiShell2Drop.set_capacity(0);
		}
		m_pPlanet->s_pShell2Generator->UnlockData();
	}
	else if (!m_iShell2VBO && !m_bGeneratingShell2)
	{
		if (m_pPlanet->GameData().GetScalableRenderOrigin().LengthSqr() < CScalableFloat(100.0f, SCALE_MEGAMETER)*CScalableFloat(100.0f, SCALE_MEGAMETER))
		{
			m_bGeneratingShell2 = true;

			CShell2GenerationJob oJob;
			oJob.pTerrain = this;
			m_pPlanet->s_pShell2Generator->AddJob(&oJob, sizeof(oJob));
		}
	}
	else if (m_iShell2VBO)
	{
		if (m_pPlanet->GameData().GetScalableRenderOrigin().LengthSqr() > CScalableFloat(1.0f, SCALE_GIGAMETER)*CScalableFloat(1.0f, SCALE_GIGAMETER))
		{
			CRenderer::UnloadVertexDataFromGL(m_iShell2VBO);
			m_iShell2VBO = 0;
		}
	}
}

size_t CPlanetTerrain::BuildTerrainArray(tvector<CTerrainPoint>& avecTerrain, size_t iDepth, const DoubleVector2D& vecMin, const DoubleVector2D& vecMax, const DoubleVector& vecCenter)
{
	size_t iQuadsPerRow = (size_t)pow(2.0f, (float)iDepth);
	size_t iVertsPerRow = iQuadsPerRow + 1;
	avecTerrain.resize(iVertsPerRow*iVertsPerRow);

	size_t iDetailLevel = (size_t)pow(2.0, (double)m_pPlanet->m_iMeterDepth);

	for (size_t x = 0; x < iVertsPerRow; x++)
	{
		double flX = RemapVal((double)x, 0, (double)iVertsPerRow-1, vecMin.x, vecMax.x);

		for (size_t y = 0; y < iVertsPerRow; y++)
		{
			double flY = RemapVal((double)y, 0, (double)iVertsPerRow-1, vecMin.y, vecMax.y);

			DoubleVector2D vecPoint(flX, flY);

			CTerrainPoint& vecTerrain = avecTerrain[y*iVertsPerRow + x];

			vecTerrain.vec2DPosition = vecPoint;

			// Offsets are generated in planet space, add them right onto the quad position.
			vecTerrain.vec3DPosition = CoordToWorld(vecPoint) + GenerateOffset(vecPoint);

//			vecTerrain.vecNormal = vecTerrain.vec3DPosition.Normalized();

			vecTerrain.vec3DPosition -= vecCenter;

			vecTerrain.vecDetail = vecTerrain.vec2DPosition * (float)iDetailLevel;
		}
	}

	for (size_t x = 0; x < iVertsPerRow; x++)
	{
		for (size_t y = 0; y < iVertsPerRow; y++)
		{
			CTerrainPoint& vecTerrain = avecTerrain[y*iVertsPerRow + x];

			Vector vecNormal(0, 0, 0);

			Vector vecUp = (vecTerrain.vec3DPosition + vecCenter).Normalized();

			if (x > 0)
			{
				CTerrainPoint& vecNeighbor = avecTerrain[y*iVertsPerRow + (x-1)];
				Vector vecToNeighbor = vecNeighbor.vec3DPosition-vecTerrain.vec3DPosition;
				Vector vecRight = -vecToNeighbor.Cross(vecUp);
				vecNormal += vecToNeighbor.Cross(vecRight);
			}

			if (x < iVertsPerRow-1)
			{
				CTerrainPoint& vecNeighbor = avecTerrain[y*iVertsPerRow + (x+1)];
				Vector vecToNeighbor = vecNeighbor.vec3DPosition-vecTerrain.vec3DPosition;
				Vector vecRight = -vecToNeighbor.Cross(vecUp);
				vecNormal += vecToNeighbor.Cross(vecRight);
			}

			if (y > 0)
			{
				CTerrainPoint& vecNeighbor = avecTerrain[(y-1)*iVertsPerRow + x];
				Vector vecToNeighbor = vecNeighbor.vec3DPosition-vecTerrain.vec3DPosition;
				Vector vecRight = -vecToNeighbor.Cross(vecUp);
				vecNormal += vecToNeighbor.Cross(vecRight);
			}

			if (y < iVertsPerRow-1)
			{
				CTerrainPoint& vecNeighbor = avecTerrain[(y+1)*iVertsPerRow + x];
				Vector vecToNeighbor = vecNeighbor.vec3DPosition-vecTerrain.vec3DPosition;
				Vector vecRight = -vecToNeighbor.Cross(vecUp);
				vecNormal += vecToNeighbor.Cross(vecRight);
			}

			vecTerrain.vecNormal = vecNormal.Normalized();
		}
	}

	return iVertsPerRow;
}

void PushVert(tvector<float>& aflVerts, const CTerrainPoint& vecPoint)
{
	aflVerts.push_back((float)vecPoint.vec3DPosition.x);
	aflVerts.push_back((float)vecPoint.vec3DPosition.y);
	aflVerts.push_back((float)vecPoint.vec3DPosition.z);
	aflVerts.push_back(vecPoint.vecNormal.x);
	aflVerts.push_back(vecPoint.vecNormal.y);
	aflVerts.push_back(vecPoint.vecNormal.z);
	aflVerts.push_back((float)vecPoint.vec2DPosition.x);	// 2D position acts as the texture coordinate
	aflVerts.push_back((float)vecPoint.vec2DPosition.y);
	aflVerts.push_back((float)vecPoint.vecDetail.x);
	aflVerts.push_back((float)vecPoint.vecDetail.y);
}

size_t BuildVerts(tvector<float>& aflVerts, const tvector<CTerrainPoint>& avecTerrain, size_t iLevels, size_t iRows)
{
	size_t iVertSize = 10;	// position, normal, two texture coords. 3 + 3 + 2 + 2 = 10

	size_t iTriangles = (int)pow(4.0f, (float)iLevels) * 2;
	aflVerts.reserve(iVertSize * iTriangles * 3);

	for (size_t x = 0; x < iRows-1; x++)
	{
		for (size_t y = 0; y < iRows-1; y++)
		{
			const CTerrainPoint& vecTerrain1 = avecTerrain[y*iRows + x];
			const CTerrainPoint& vecTerrain2 = avecTerrain[y*iRows + (x+1)];
			const CTerrainPoint& vecTerrain3 = avecTerrain[(y+1)*iRows + (x+1)];
			const CTerrainPoint& vecTerrain4 = avecTerrain[(y+1)*iRows + x];

			PushVert(aflVerts, vecTerrain1);
			PushVert(aflVerts, vecTerrain2);
			PushVert(aflVerts, vecTerrain3);

			PushVert(aflVerts, vecTerrain1);
			PushVert(aflVerts, vecTerrain3);
			PushVert(aflVerts, vecTerrain4);
		}
	}

	return iTriangles;
}

size_t CPlanetTerrain::BuildIndexedVerts(tvector<float>& aflVerts, tvector<unsigned int>& aiIndices, const tvector<CTerrainPoint>& avecTerrain, size_t iLevels, size_t iRows)
{
	size_t iVertSize = 10;	// position, normal, two texture coords. 3 + 3 + 2 + 2 = 10

	size_t iTriangles = (int)pow(4.0f, (float)iLevels) * 2;
	aiIndices.reserve(iTriangles * 3);

	aflVerts.reserve(avecTerrain.size()*iVertSize);

	size_t iTerrainSize = avecTerrain.size();
	for (size_t x = 0; x < iTerrainSize; x++)
		PushVert(aflVerts, avecTerrain[x]);

	for (size_t x = 0; x < iRows-1; x++)
	{
		for (size_t y = 0; y < iRows-1; y++)
		{
			unsigned int i1 = y*iRows + x;
			unsigned int i2 = y*iRows + (x+1);
			unsigned int i3 = (y+1)*iRows + (x+1);
			unsigned int i4 = (y+1)*iRows + x;

			TAssert(i1 < iTerrainSize);
			TAssert(i2 < iTerrainSize);
			TAssert(i3 < iTerrainSize);
			TAssert(i4 < iTerrainSize);

			aiIndices.push_back(i1);
			aiIndices.push_back(i2);
			aiIndices.push_back(i3);

			aiIndices.push_back(i1);
			aiIndices.push_back(i3);
			aiIndices.push_back(i4);
		}
	}

	return iTriangles;
}

void CPlanetTerrain::CreateShell1VBO()
{
	if (m_iShell1VBO)
		CRenderer::UnloadVertexDataFromGL(m_iShell1VBO);

	m_iShell1VBO = 0;

	// Wouldn't it be cool if some planets were like, cubeworld?
	//int iHighLevels = 0;

	int iHighLevels = 3;

	tvector<CTerrainPoint> avecTerrain;
	size_t iRows = BuildTerrainArray(avecTerrain, iHighLevels, Vector2D(0, 0), Vector2D(1, 1), Vector(0, 0, 0));

	tvector<float> aflVerts;
	size_t iTriangles = BuildVerts(aflVerts, avecTerrain, iHighLevels, iRows);

	m_iShell1VBO = CRenderer::LoadVertexDataIntoGL(aflVerts.size() * sizeof(float), aflVerts.data());
	m_iShell1VBOSize = iTriangles * 3;
}

void CPlanetTerrain::CreateShell2VBO()
{
	if (m_iShell2VBO)
	{
		CRenderer::UnloadVertexDataFromGL(m_iShell2VBO);
		CRenderer::UnloadVertexDataFromGL(m_iShell2IBO);
	}

	m_iShell2VBO = 0;

	int iHighLevels = m_pPlanet->ChunkDepth();

	tvector<CTerrainPoint> avecTerrain;
	size_t iRows = BuildTerrainArray(avecTerrain, iHighLevels, Vector2D(0, 0), Vector2D(1, 1), Vector(0, 0, 0));

	tvector<float> aflVerts;
	tvector<unsigned int> aiVerts;
	size_t iTriangles = BuildIndexedVerts(aflVerts, aiVerts, avecTerrain, iHighLevels, iRows);
	m_iShell2IBOSize = iTriangles*3;

	// Can't use the current GL context to create a VBO in this thread, so send the info to a drop where the main thread can pick it up.
	m_pPlanet->s_pShell2Generator->LockData();
	TAssert(!m_aflShell2Drop.size());
	swap(m_aflShell2Drop, aflVerts);
	swap(m_aiShell2Drop, aiVerts);
	m_pPlanet->s_pShell2Generator->UnlockData();
}

void CPlanetTerrain::Render(class CRenderingContext* c) const
{
	double flDistance = m_pPlanet->m_vecCharacterLocalOrigin.Length();
	Vector vecPlayerDirection = m_pPlanet->m_vecCharacterLocalOrigin / flDistance;

	float flPlayerDot = vecPlayerDirection.Dot(GetDirection());
	if (flPlayerDot < -0.60f)
		return;

	bool bLowRes = flDistance > 300;

	CPlayerCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
	if (pCharacter->GetNearestPlanet())
	{
		if (flPlayerDot < 0.45f)
			return;
	}

	if (!m_bGeneratingShell2 && m_iShell2VBO && !bLowRes)
	{
		c->BeginRenderVertexArray(m_iShell2VBO);
		c->SetPositionBuffer(0u, 10*sizeof(float));
		c->SetNormalsBuffer(3*sizeof(float), 10*sizeof(float));
		c->SetTexCoordBuffer(6*sizeof(float), 10*sizeof(float), 0);
		c->SetTexCoordBuffer(8*sizeof(float), 10*sizeof(float), 1);
		c->EndRenderVertexArrayIndexed(m_iShell2IBO, m_iShell2IBOSize);
	}
	else
	{
		c->BeginRenderVertexArray(m_iShell1VBO);
		c->SetPositionBuffer(0u, 10*sizeof(float));
		c->SetNormalsBuffer(3*sizeof(float), 10*sizeof(float));
		c->SetTexCoordBuffer(6*sizeof(float), 10*sizeof(float), 0);
		c->SetTexCoordBuffer(8*sizeof(float), 10*sizeof(float), 1);
		c->EndRenderVertexArray(m_iShell1VBOSize);
	}
}

DoubleVector CPlanetTerrain::GenerateOffset(const DoubleVector2D& vecCoordinate)
{
	DoubleVector2D vecAdjustedCoordinate = vecCoordinate;
	Vector vecDirection = GetDirection();
	if (vecDirection[0] > 0)
		vecAdjustedCoordinate += DoubleVector2D(0, 0);
	else if (vecDirection[0] < 0)
		vecAdjustedCoordinate += DoubleVector2D(2, 0);
	else if (vecDirection[1] > 0)
		vecAdjustedCoordinate += DoubleVector2D(0, 1);
	else if (vecDirection[1] < 0)
		vecAdjustedCoordinate += DoubleVector2D(0, -1);
	else if (vecDirection[2] > 0)
		vecAdjustedCoordinate += DoubleVector2D(3, 0);
	else
		vecAdjustedCoordinate += DoubleVector2D(1, 0);

	DoubleVector vecOffset;
	double flSpaceFactor = 1.1/2;
	double flHeightFactor = 0.035;
	for (size_t i = 0; i < TERRAIN_NOISE_ARRAY_SIZE; i++)
	{
		double flXFactor = vecAdjustedCoordinate.x * flSpaceFactor;
		double flYFactor = vecAdjustedCoordinate.y * flSpaceFactor;
		double x = m_pPlanet->m_aNoiseArray[i][0].Noise(flXFactor, flYFactor) * flHeightFactor;
		double y = m_pPlanet->m_aNoiseArray[i][1].Noise(flXFactor, flYFactor) * flHeightFactor;
		double z = m_pPlanet->m_aNoiseArray[i][2].Noise(flXFactor, flYFactor) * flHeightFactor;

		vecOffset += DoubleVector(x, y, z);

		flSpaceFactor = flSpaceFactor*2;
		flHeightFactor = flHeightFactor/1.4;
	}

	return vecOffset;
}

bool CPlanetTerrain::FindChunkNearestToPlayer(DoubleVector2D& vecChunkMin, DoubleVector2D& vecChunkMax)
{
	size_t iChunkDepth = m_pPlanet->ChunkDepth();
	DoubleVector vecLocalPlayer = m_pPlanet->m_vecCharacterLocalOrigin;

	DoubleVector2D vecMinCoord(0, 0);
	DoubleVector2D vecMaxCoord(1, 1);

	for (size_t i = 0; i < iChunkDepth; i++)
	{
		bool bFound = false;
		double flLowestDistanceSqr = FLT_MAX;
		DoubleVector2D vecNearestQuadMin;
		DoubleVector2D vecNearestQuadMax;

		for (size_t j = 0; j < 4; j++)
		{
			DoubleVector2D vecQuadMin;
			DoubleVector2D vecQuadMax;

			if (j == 0)
			{
				vecQuadMin = vecMinCoord;
				vecQuadMax = (vecMinCoord + vecMaxCoord)/2;
			}
			else if (j == 1)
			{
				vecQuadMin = DoubleVector2D(vecMinCoord.x, (vecMinCoord.y + vecMaxCoord.y)/2);
				vecQuadMax = DoubleVector2D((vecMinCoord.x + vecMaxCoord.x)/2, vecMaxCoord.y);
			}
			else if (j == 2)
			{
				vecQuadMin = DoubleVector2D((vecMinCoord.x + vecMaxCoord.x)/2, vecMinCoord.y);
				vecQuadMax = DoubleVector2D(vecMaxCoord.x, (vecMinCoord.y + vecMaxCoord.y)/2);
			}
			else
			{
				vecQuadMin = (vecMinCoord + vecMaxCoord)/2;
				vecQuadMax = vecMaxCoord;
			}

			DoubleVector vecMinWorld = CoordToWorld(vecQuadMin) + GenerateOffset(vecQuadMin);
			DoubleVector vecMaxWorld = CoordToWorld(vecQuadMax) + GenerateOffset(vecQuadMax);

			TemplateAABB<double> aabbWorld(vecMinWorld, vecMaxWorld);

			double flDistanceSqr = aabbWorld.Center().DistanceSqr(vecLocalPlayer);
			if (flDistanceSqr < flLowestDistanceSqr && flDistanceSqr < aabbWorld.Size().LengthSqr())
			{
				bFound = true;

				flLowestDistanceSqr = flDistanceSqr;

				vecNearestQuadMin = vecQuadMin;
				vecNearestQuadMax = vecQuadMax;
			}
		}

		if (!bFound)
			return false;

		vecMinCoord = vecNearestQuadMin;
		vecMaxCoord = vecNearestQuadMax;
	}

	vecChunkMin = vecMinCoord;
	vecChunkMax = vecMaxCoord;

	return true;
}

DoubleVector CPlanetTerrain::CoordToWorld(const DoubleVector2D& v) const
{
	DoubleVector vecDirection = GetDirection();

	DoubleVector vecCubePoint;

	double x = RemapVal(v.x, 0, 1, -1, 1);
	double y = RemapVal(v.y, 0, 1, -1, 1);

	if (vecDirection[0] > 0)
		vecCubePoint = DoubleVector(vecDirection[0], y, -x);
	else if (vecDirection[0] < 0)
		vecCubePoint = DoubleVector(vecDirection[0], y, x);
	else if (vecDirection[1] > 0)
		vecCubePoint = DoubleVector(-y, vecDirection[1], -x);
	else if (vecDirection[1] < 0)
		vecCubePoint = DoubleVector(y, vecDirection[1], -x);
	else if (vecDirection[2] > 0)
		vecCubePoint = DoubleVector(x, y, vecDirection[2]);
	else
		vecCubePoint = DoubleVector(-x, y, vecDirection[2]);

	return vecCubePoint.Normalized() * m_pPlanet->GetRadius().GetUnits(m_pPlanet->GetScale());
}
