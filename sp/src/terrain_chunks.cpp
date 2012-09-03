#include "terrain_chunks.h"

#include <parallelize.h>

#include <renderer/renderingcontext.h>
#include <tinker/cvar.h>

#include "sp_game.h"
#include "sp_playercharacter.h"
#include "sp_renderer.h"
#include "star.h"
#include "planet.h"
#include "planet_terrain.h"

CParallelizer* CTerrainChunkManager::s_pChunkGenerator = nullptr;

void GenerateChunkCallback(void* pJob)
{
	CChunkGenerationJob* pChunkJob = reinterpret_cast<CChunkGenerationJob*>(pJob);

	pChunkJob->pManager->GenerateChunk(pChunkJob->iChunk);
}

CTerrainChunkManager::CTerrainChunkManager(CPlanet* pPlanet)
{
	if (!s_pChunkGenerator)
	{
		s_pChunkGenerator = new CParallelizer(GenerateChunkCallback, 1);
		s_pChunkGenerator->Start();
	}

	m_pPlanet = pPlanet;

	m_flNextChunkCheck = 0;
}

void CTerrainChunkManager::AddChunk(size_t iTerrain, const DoubleVector2D& vecChunkMin, const DoubleVector2D& vecChunkMax)
{
	size_t iIndex = ~0;
	for (size_t i = 0; i < m_apChunks.size(); i++)
	{
		CTerrainChunk* pChunk = m_apChunks[i];
		if (pChunk)
		{
			if (pChunk->m_iTerrain == iTerrain && pChunk->m_vecMin == vecChunkMin && pChunk->m_vecMax == vecChunkMax)
				return;
		}
		else if (!pChunk)
			iIndex = i;
	}

	if (iIndex == ~0)
	{
		iIndex = m_apChunks.size();
		m_apChunks.push_back();
	}

	m_apChunks[iIndex] = new CTerrainChunk(this, iIndex, iTerrain, vecChunkMin, vecChunkMax);
	m_apChunks[iIndex]->Initialize();
}

void CTerrainChunkManager::RemoveChunk(size_t iChunk)
{
	if (iChunk == ~0)
	{
		TAssert(iChunk != ~0);
		return;
	}

	s_pChunkGenerator->LockData();
	RemoveChunkNoLock(iChunk);
	s_pChunkGenerator->UnlockData();
}

void CTerrainChunkManager::RemoveChunkNoLock(size_t iChunk)
{
	if (m_apChunks[iChunk] && m_apChunks[iChunk]->m_bGeneratingLowRes)
	{
		s_pChunkGenerator->UnlockData();
		return;
	}

	delete m_apChunks[iChunk];
	m_apChunks[iChunk] = NULL;
}

CTerrainChunk* CTerrainChunkManager::GetChunk(size_t iChunk) const
{
	if (iChunk >= m_apChunks.size())
		return NULL;

	return m_apChunks[iChunk];
}

CVar terrain_chunkcheck("terrain_chunkcheck", "1");

void CTerrainChunkManager::Think()
{
	CPlayerCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
	if (pCharacter->GetNearestPlanet() != m_pPlanet)
	{
		if (s_pChunkGenerator->TryLockData())
		{
			size_t iChunksRemaining = 0;
			for (size_t i = 0; i < m_apChunks.size(); i++)
			{
				if (m_apChunks[i])
				{
					if (m_apChunks[i]->m_bGeneratingLowRes)
					{
						iChunksRemaining++;
						continue;
					}

					RemoveChunkNoLock(i);
				}
			}

			if (iChunksRemaining == 0)
				m_apChunks.set_capacity(0);
			s_pChunkGenerator->UnlockData();
		}
		return;
	}

	if (GameServer()->GetGameTime() > m_flNextChunkCheck)
	{
		AddNearbyChunks();

		m_flNextChunkCheck = GameServer()->GetGameTime() + terrain_chunkcheck.GetFloat();
	}

	for (size_t i = 0; i < m_apChunks.size(); i++)
	{
		CTerrainChunk* pChunk = m_apChunks[i];
		if (!pChunk)
			continue;

		if (m_pPlanet->m_vecCharacterLocalOrigin.DistanceSqr(pChunk->m_aabbBounds.Center()) > pChunk->m_aabbBounds.Size().LengthSqr())
			RemoveChunk(i);

		// If the chunk wasn't removed, let it think.
		pChunk = m_apChunks[i];
		if (!pChunk)
			continue;

		pChunk->Think();
	}
}

void CTerrainChunkManager::AddNearbyChunks()
{
	for (size_t i = 0; i < 6; i++)
	{
		DoubleVector2D vecChunkMin;
		DoubleVector2D vecChunkMax;

		if (!m_pPlanet->m_apTerrain[i]->FindChunkNearestToPlayer(vecChunkMin, vecChunkMax))
			continue;

		AddChunk(i, vecChunkMin, vecChunkMax);
	}
}

void CTerrainChunkManager::GenerateChunk(size_t iChunk)
{
	s_pChunkGenerator->LockData();
		TAssert(iChunk >= 0 && iChunk < m_apChunks.size());
		TAssert(m_apChunks[iChunk]);

		if (iChunk >= m_apChunks.size() || !m_apChunks[iChunk])
		{
			s_pChunkGenerator->UnlockData();
			return;
		}

		m_apChunks[iChunk]->m_bGeneratingLowRes = true;
	s_pChunkGenerator->UnlockData();

	m_apChunks[iChunk]->GenerateTerrain();
}

CVar r_markchunks("r_markchunks", "off");

void CTerrainChunkManager::Render()
{
	if (r_markchunks.GetBool() && SPGame()->GetLocalPlayerCharacter()->GetNearestPlanet() == m_pPlanet)
	{
		for (size_t i = 0; i < m_apChunks.size(); i++)
		{
			CTerrainChunk* pChunk = m_apChunks[i];
			if (!pChunk)
				continue;

			DoubleVector vecMinWorld = m_pPlanet->m_apTerrain[pChunk->m_iTerrain]->CoordToWorld(pChunk->m_vecMin) + m_pPlanet->m_apTerrain[pChunk->m_iTerrain]->GenerateOffset(pChunk->m_vecMin);
			DoubleVector vecMaxWorld = m_pPlanet->m_apTerrain[pChunk->m_iTerrain]->CoordToWorld(pChunk->m_vecMax) + m_pPlanet->m_apTerrain[pChunk->m_iTerrain]->GenerateOffset(pChunk->m_vecMax);

			CRenderingContext c(GameServer()->GetRenderer(), true);

			c.SetDepthTest(false);
			c.RenderWireBox(AABB(vecMinWorld, vecMaxWorld));
		}
	}

	for (size_t i = 0; i < m_apChunks.size(); i++)
	{
		if (!m_apChunks[i])
			continue;
		
		m_apChunks[i]->Render();
	}
}

CTerrainChunk::CTerrainChunk(CTerrainChunkManager* pManager, size_t iChunk, size_t iTerrain, const DoubleVector2D& vecMin, const DoubleVector2D& vecMax)
{
	m_pManager = pManager;
	m_iChunk = iChunk;
	m_iTerrain = iTerrain;
	m_vecMin = vecMin;
	m_vecMax = vecMax;

	CPlanetTerrain* pTerrain = m_pManager->m_pPlanet->GetTerrain(m_iTerrain);

	DoubleVector vecWorldMin = pTerrain->CoordToWorld(m_vecMin) + pTerrain->GenerateOffset(m_vecMin);
	DoubleVector vecWorldMax = pTerrain->CoordToWorld(m_vecMax) + pTerrain->GenerateOffset(m_vecMax);
	m_aabbBounds = TemplateAABB<double>(vecWorldMin, vecWorldMax);

	m_iLowResTerrainVBO = 0;
}

CTerrainChunk::~CTerrainChunk()
{
	if (m_iLowResTerrainVBO)
	{
		CRenderer::UnloadVertexDataFromGL(m_iLowResTerrainVBO);
		CRenderer::UnloadVertexDataFromGL(m_iLowResTerrainIBO);
	}
}

void CTerrainChunk::Initialize()
{
	CChunkGenerationJob oJob;
	oJob.pManager = m_pManager;
	oJob.iChunk = m_iChunk;
	m_pManager->s_pChunkGenerator->AddJob(&oJob, sizeof(oJob));
}

void CTerrainChunk::Think()
{
	if (m_bGeneratingLowRes)
	{
		if (m_pManager->s_pChunkGenerator->TryLockData())
		{
			if (m_aflLowResDrop.size())
			{
				m_iLowResTerrainVBO = CRenderer::LoadVertexDataIntoGL(m_aflLowResDrop.size() * sizeof(float), m_aflLowResDrop.data());
				m_iLowResTerrainIBO = CRenderer::LoadIndexDataIntoGL(m_aiLowResDrop.size() * sizeof(unsigned int), m_aiLowResDrop.data());
				m_bGeneratingLowRes = false;
				m_aflLowResDrop.set_capacity(0);
				m_aiLowResDrop.set_capacity(0);
			}
			m_pManager->s_pChunkGenerator->UnlockData();
		}
	}
}

void CTerrainChunk::GenerateTerrain()
{
	if (m_iLowResTerrainVBO)
	{
		CRenderer::UnloadVertexDataFromGL(m_iLowResTerrainVBO);
		CRenderer::UnloadVertexDataFromGL(m_iLowResTerrainIBO);
	}

	m_iLowResTerrainVBO = 0;

	int iHighestLevel = m_pManager->m_pPlanet->PhysicsDepth();
	int iLowestLevel = m_pManager->m_pPlanet->ChunkDepth();

	int iResolution = iHighestLevel-iLowestLevel;

	CPlanetTerrain* pTerrain = m_pManager->m_pPlanet->GetTerrain(m_iTerrain);

	DoubleVector2D vecCoordCenter = (m_vecMin + m_vecMax)/2;
	m_vecLocalCenter = pTerrain->CoordToWorld(vecCoordCenter) + pTerrain->GenerateOffset(vecCoordCenter);

	tvector<CTerrainPoint> avecTerrain;
	size_t iRows = pTerrain->BuildTerrainArray(avecTerrain, iResolution, m_vecMin, m_vecMax, m_vecLocalCenter);

	tvector<float> aflVerts;
	tvector<unsigned int> aiVerts;
	size_t iTriangles = CPlanetTerrain::BuildIndexedVerts(aflVerts, aiVerts, avecTerrain, iResolution, iRows);
	m_iLowResTerrainIBOSize = iTriangles*3;

	// Can't use the current GL context to create a VBO in this thread, so send the info to a drop where the main thread can pick it up.
	m_pManager->s_pChunkGenerator->LockData();
		TAssert(!m_aflLowResDrop.size());
		swap(m_aflLowResDrop, aflVerts);
		swap(m_aiLowResDrop, aiVerts);
	m_pManager->s_pChunkGenerator->UnlockData();
}

void CTerrainChunk::Render()
{
	if (!m_iLowResTerrainVBO)
		return;

	scale_t eRenderScale = SPGame()->GetSPRenderer()->GetRenderingScale();
	if (eRenderScale >= SCALE_MEGAMETER)
		return;

	CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
	CPlanet* pPlanet = m_pManager->m_pPlanet;
	CStar* pStar = SPGame()->GetSPRenderer()->GetClosestStar();
	scale_t ePlanetScale = pPlanet->GetScale();
	CScalableVector vecCharacterOrigin = pCharacter->GetLocalOrigin();
	CScalableMatrix mPlanetTransform = pPlanet->GetGlobalTransform();

	CScalableVector vecChunkCenter = mPlanetTransform.TransformVector(CScalableVector(m_vecLocalCenter, ePlanetScale) - vecCharacterOrigin);

	CScalableMatrix mChunkTransform = mPlanetTransform;
	mChunkTransform.SetTranslation(vecChunkCenter);

	Matrix4x4 mChunkTransformMeters = mChunkTransform.GetUnits(eRenderScale);

	float flScale;
	if (eRenderScale == SCALE_RENDER)
		flScale = (float)CScalableFloat::ConvertUnits(1, ePlanetScale, SCALE_METER);
	else
		flScale = (float)CScalableFloat::ConvertUnits(1, ePlanetScale, eRenderScale);

	CScalableMatrix mPlanetToLocal = mPlanetTransform.InvertedRT();
	Vector vecStarLightPosition = (mPlanetToLocal.TransformVector(pStar->GameData().GetScalableRenderOrigin())).GetUnits(eRenderScale);

	CRenderingContext r(GameServer()->GetRenderer(), true);

	r.ResetTransformations();

	r.Transform(mChunkTransformMeters);
	r.Scale(flScale, flScale, flScale);

	r.SetUniform("vecStarLightPosition", vecStarLightPosition);

	r.BeginRenderVertexArray(m_iLowResTerrainVBO);
	r.SetPositionBuffer(0u, 10*sizeof(float));
	r.SetNormalsBuffer(3*sizeof(float), 10*sizeof(float));
	r.SetTexCoordBuffer(6*sizeof(float), 10*sizeof(float), 0);
	r.SetTexCoordBuffer(8*sizeof(float), 10*sizeof(float), 1);
	r.EndRenderVertexArrayIndexed(m_iLowResTerrainIBO, m_iLowResTerrainIBOSize);
}
