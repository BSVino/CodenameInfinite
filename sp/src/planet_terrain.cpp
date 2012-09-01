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
			m_iShell2IBO = CRenderer::LoadIndexDataIntoGL(m_aiShell2Drop.size() * sizeof(unsigned short), m_aiShell2Drop.data());
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

size_t CPlanetTerrain::BuildTerrainArray(tvector<CTerrainPoint>& avecTerrain, size_t iDepth, const Vector2D& vecMin, const Vector2D vecMax)
{
	size_t iQuadsPerRow = (size_t)pow(2.0f, (float)iDepth);
	size_t iVertsPerRow = iQuadsPerRow + 1;
	avecTerrain.resize(iVertsPerRow*iVertsPerRow);

	size_t iDetailLevel = (size_t)pow(2.0, (double)iDepth);

	for (size_t x = 0; x < iVertsPerRow; x++)
	{
		float flX = RemapVal((float)x, 0, (float)iVertsPerRow-1, vecMin.x, vecMax.x);

		for (size_t y = 0; y < iVertsPerRow; y++)
		{
			float flY = RemapVal((float)y, 0, (float)iVertsPerRow-1, vecMin.y, vecMax.y);

			Vector2D vecPoint(flX, flY);

			CTerrainPoint& vecTerrain = avecTerrain[y*iVertsPerRow + x];

			vecTerrain.vec2DPosition = vecPoint;

			vecTerrain.vecOffset = GenerateOffset(vecPoint);

			// Offsets are generated in planet space, add them right onto the quad position.
			vecTerrain.vec3DPosition = CoordToWorld(vecPoint) + vecTerrain.vecOffset;

			vecTerrain.vecNormal = vecTerrain.vec3DPosition.Normalized();

			vecTerrain.vecDetail = vecTerrain.vec2DPosition * (float)iDetailLevel;
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

size_t BuildIndexedVerts(tvector<float>& aflVerts, tvector<unsigned short>& aiIndices, const tvector<CTerrainPoint>& avecTerrain, size_t iLevels, size_t iRows)
{
	size_t iVertSize = 10;	// position, normal, two texture coords. 3 + 3 + 2 + 2 = 10

	size_t iTriangles = (int)pow(4.0f, (float)iLevels) * 2;
	aiIndices.reserve(iTriangles * 3);

	aflVerts.reserve(avecTerrain.size());

	size_t iTerrainSize = avecTerrain.size();
	for (size_t x = 0; x < iTerrainSize; x++)
		PushVert(aflVerts, avecTerrain[x]);

	for (size_t x = 0; x < iRows-1; x++)
	{
		for (size_t y = 0; y < iRows-1; y++)
		{
			unsigned short i1 = y*iRows + x;
			unsigned short i2 = y*iRows + (x+1);
			unsigned short i3 = (y+1)*iRows + (x+1);
			unsigned short i4 = (y+1)*iRows + x;

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
	size_t iRows = BuildTerrainArray(avecTerrain, iHighLevels, Vector2D(0, 0), Vector2D(1, 1));

	tvector<float> aflVerts;
	size_t iTriangles = BuildVerts(aflVerts, avecTerrain, iHighLevels, iRows);

	m_iShell1VBO = CRenderer::LoadVertexDataIntoGL(aflVerts.size() * sizeof(float), aflVerts.data());
	m_iShell1VBOSize = iTriangles * 3;
}

void CPlanetTerrain::CreateShell2VBO()
{
	if (m_iShell2VBO)
		CRenderer::UnloadVertexDataFromGL(m_iShell2VBO);

	m_iShell2VBO = 0;

	int iHighLevels = m_pPlanet->ChunkDepth();

	tvector<CTerrainPoint> avecTerrain;
	size_t iRows = BuildTerrainArray(avecTerrain, iHighLevels, Vector2D(0, 0), Vector2D(1, 1));

	tvector<float> aflVerts;
	tvector<unsigned short> aiVerts;
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

DoubleVector2D CPlanetTerrain::WorldToCoord(const DoubleVector& v) const
{
	// This hasn't been updated to match QuadToWorldTree()
	TAssert(false);

	// Find the spot on the planet's surface that this point is closest to.
	DoubleVector vecPlanetOrigin = m_pPlanet->GetGlobalOrigin();
	DoubleVector vecPointDirection = (v - vecPlanetOrigin).Normalized();

	DoubleVector vecDirection = GetDirection();

	if (vecPointDirection.Dot(vecDirection) <= 0)
		return DoubleVector2D(-1, -1);

	int iBig;
	int iX;
	int iY;
	if (vecDirection[0] != 0)
	{
		iBig = 0;
		iX = 1;
		iY = 2;
	}
	else if (vecDirection[1] != 0)
	{
		iBig = 1;
		iX = 0;
		iY = 2;
	}
	else
	{
		iBig = 2;
		iX = 0;
		iY = 1;
	}

	// Shoot a ray from (0,0,0) through (a,b,c) until it hits a face of the bounding cube.
	// The faces of the bounding cube are specified by GetDirection() above.
	// If the ray hits the +z face at (x,y,z) then (x,y,1) is a scalar multiple of (a,b,c).
	// (x,y,1) = r*(a,b,c), so 1 = r*c and r = 1/c
	// Then we can figure x and y with x = a*r and y = b*r

	double r = 1/vecPointDirection[iBig];

	double x = RemapVal(vecPointDirection[iX]*r, -1, 1, 0, 1);
	double y = RemapVal(vecPointDirection[iY]*r, -1, 1, 0, 1);

	return DoubleVector2D(x, y);
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
