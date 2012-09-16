#include "terrain_lumps.h"

#include <parallelize.h>

#include <renderer/renderingcontext.h>
#include <tinker/cvar.h>

#include "sp_game.h"
#include "sp_playercharacter.h"
#include "sp_renderer.h"
#include "star.h"
#include "planet.h"
#include "planet_terrain.h"
#include "terrain_chunks.h"

CParallelizer* CTerrainLumpManager::s_pLumpGenerator = nullptr;

void GenerateLumpCallback(void* pJob)
{
	CLumpGenerationJob* pLumpJob = reinterpret_cast<CLumpGenerationJob*>(pJob);

	pLumpJob->pManager->GenerateLump(pLumpJob->iLump);
}

CTerrainLumpManager::CTerrainLumpManager(CPlanet* pPlanet)
{
	if (!s_pLumpGenerator)
	{
		s_pLumpGenerator = new CParallelizer(GenerateLumpCallback, 1);
		s_pLumpGenerator->Start();
	}

	m_pPlanet = pPlanet;

	m_flNextLumpCheck = 0;
}

void CTerrainLumpManager::AddLump(size_t iTerrain, const DoubleVector2D& vecLumpMin, const DoubleVector2D& vecLumpMax)
{
	size_t iIndex = ~0;
	for (size_t i = 0; i < m_apLumps.size(); i++)
	{
		CTerrainLump* pLump = m_apLumps[i];
		if (pLump)
		{
			if (pLump->m_iTerrain == iTerrain && pLump->m_vecMin == vecLumpMin && pLump->m_vecMax == vecLumpMax)
				return;
		}
		else if (!pLump)
			iIndex = i;
	}

	if (iIndex == ~0)
	{
		iIndex = m_apLumps.size();
		m_apLumps.push_back();
	}

	m_apLumps[iIndex] = new CTerrainLump(this, iIndex, iTerrain, vecLumpMin, vecLumpMax);
	m_apLumps[iIndex]->Initialize();
}

void CTerrainLumpManager::RemoveLump(size_t iLump)
{
	if (iLump == ~0)
	{
		TAssert(iLump != ~0);
		return;
	}

	CMutexLocker oLock = s_pLumpGenerator->GetLock();
	oLock.Lock();
	RemoveLumpNoLock(iLump);
}

void CTerrainLumpManager::RemoveLumpNoLock(size_t iLump)
{
	if (m_apLumps[iLump] && m_apLumps[iLump]->m_bGeneratingLowRes)
		return;

	delete m_apLumps[iLump];
	m_apLumps[iLump] = NULL;

	m_pPlanet->m_pChunkManager->LumpRemoved(iLump);
}

CTerrainLump* CTerrainLumpManager::GetLump(size_t iLump) const
{
	if (iLump >= m_apLumps.size())
		return NULL;

	return m_apLumps[iLump];
}

CVar terrain_lumpcheck("terrain_lumpcheck", "1");

void CTerrainLumpManager::Think()
{
	CPlayerCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
	if (pCharacter->GetNearestPlanet() != m_pPlanet)
	{
		CMutexLocker oLock = s_pLumpGenerator->GetLock();
		if (oLock.TryLock())
		{
			size_t iLumpsRemaining = 0;
			tvector<size_t> aiRebuildTerrains;

			for (size_t i = 0; i < m_apLumps.size(); i++)
			{
				if (m_apLumps[i])
				{
					if (m_apLumps[i]->m_bGeneratingLowRes)
					{
						iLumpsRemaining++;
						continue;
					}

					bool bAdd = true;
					for (size_t j = 0; j < aiRebuildTerrains.size(); j++)
					{
						if (aiRebuildTerrains[j] == m_apLumps[i]->GetTerrain())
						{
							bAdd = false;
							break;
						}
					}

					if (bAdd)
						aiRebuildTerrains.push_back(m_apLumps[i]->GetTerrain());

					RemoveLumpNoLock(i);
				}
			}

			for (size_t i = 0; i < aiRebuildTerrains.size(); i++)
				m_pPlanet->GetTerrain(aiRebuildTerrains[i])->RebuildShell2Indices();

			if (iLumpsRemaining == 0)
				m_apLumps.set_capacity(0);
		}
		return;
	}

	if (GameServer()->GetGameTime() > m_flNextLumpCheck)
	{
		AddNearbyLumps();

		m_flNextLumpCheck = GameServer()->GetGameTime() + terrain_lumpcheck.GetFloat();
	}

	tvector<size_t> aiRebuildTerrains;
	for (size_t i = 0; i < m_apLumps.size(); i++)
	{
		CTerrainLump* pLump = m_apLumps[i];
		if (!pLump)
			continue;

		if (m_pPlanet->m_vecCharacterLocalOrigin.DistanceSqr(pLump->m_aabbBounds.Center()) > pLump->m_aabbBounds.Size().LengthSqr()*4)
		{
			bool bAdd = true;
			for (size_t j = 0; j < aiRebuildTerrains.size(); j++)
			{
				if (aiRebuildTerrains[j] == m_apLumps[i]->GetTerrain())
				{
					bAdd = false;
					break;
				}
			}

			if (bAdd)
				aiRebuildTerrains.push_back(m_apLumps[i]->GetTerrain());

			RemoveLump(i);
		}

		// If the Lump wasn't removed, let it think.
		pLump = m_apLumps[i];
		if (!pLump)
			continue;

		pLump->Think();
	}

	for (size_t i = 0; i < aiRebuildTerrains.size(); i++)
		m_pPlanet->GetTerrain(aiRebuildTerrains[i])->RebuildShell2Indices();
}

void CTerrainLumpManager::AddNearbyLumps()
{
	for (size_t i = 0; i < 6; i++)
	{
		DoubleVector2D vecLumpMin;
		DoubleVector2D vecLumpMax;

		if (!m_pPlanet->m_apTerrain[i]->FindAreaNearestToPlayer(m_pPlanet->LumpDepth(), DoubleVector2D(0, 0), DoubleVector2D(1, 1), vecLumpMin, vecLumpMax))
			continue;

		AddLump(i, vecLumpMin, vecLumpMax);
	}
}

void CTerrainLumpManager::GenerateLump(size_t iLump)
{
	CMutexLocker oLock = s_pLumpGenerator->GetLock();
	oLock.Lock();
		TAssert(iLump >= 0 && iLump < m_apLumps.size());
		TAssert(m_apLumps[iLump]);

		if (iLump >= m_apLumps.size() || !m_apLumps[iLump])
			return;

		m_apLumps[iLump]->m_bGeneratingLowRes = true;
	oLock.Unlock();

	m_apLumps[iLump]->GenerateTerrain();
}

CVar r_marklumps("r_marklumps", "off");

void CTerrainLumpManager::Render()
{
	if (r_marklumps.GetBool() && SPGame()->GetLocalPlayerCharacter()->GetNearestPlanet() == m_pPlanet)
	{
		for (size_t i = 0; i < m_apLumps.size(); i++)
		{
			CTerrainLump* pLump = m_apLumps[i];
			if (!pLump)
				continue;

			DoubleVector vecMinWorld = m_pPlanet->m_apTerrain[pLump->m_iTerrain]->CoordToWorld(pLump->m_vecMin) + m_pPlanet->m_apTerrain[pLump->m_iTerrain]->GenerateOffset(pLump->m_vecMin);
			DoubleVector vecMaxWorld = m_pPlanet->m_apTerrain[pLump->m_iTerrain]->CoordToWorld(pLump->m_vecMax) + m_pPlanet->m_apTerrain[pLump->m_iTerrain]->GenerateOffset(pLump->m_vecMax);

			CRenderingContext c(GameServer()->GetRenderer(), true);

			c.SetDepthTest(false);
			c.RenderWireBox(AABB(vecMinWorld, vecMaxWorld));
		}
	}

	for (size_t i = 0; i < m_apLumps.size(); i++)
	{
		if (!m_apLumps[i])
			continue;
		
		m_apLumps[i]->Render();
	}
}

CTerrainLump::CTerrainLump(CTerrainLumpManager* pManager, size_t iLump, size_t iTerrain, const DoubleVector2D& vecMin, const DoubleVector2D& vecMax)
{
	m_pManager = pManager;
	m_iLump = iLump;
	m_iTerrain = iTerrain;
	m_vecMin = vecMin;
	m_vecMax = vecMax;

	CPlanetTerrain* pTerrain = m_pManager->m_pPlanet->GetTerrain(m_iTerrain);

	DoubleVector vecWorldMin = pTerrain->CoordToWorld(m_vecMin) + pTerrain->GenerateOffset(m_vecMin);
	DoubleVector vecWorldMax = pTerrain->CoordToWorld(m_vecMax) + pTerrain->GenerateOffset(m_vecMax);
	m_aabbBounds = TemplateAABB<double>(vecWorldMin, vecWorldMax);

	m_iLowResTerrainVBO = 0;

	size_t iResolution = (size_t)pow(2.0f, (float)m_pManager->m_pPlanet->LumpDepth());
	m_iX = (size_t)RemapVal((vecMin.x+vecMax.x)/2, 0, 1, 0, iResolution);
	m_iY = (size_t)RemapVal((vecMin.y+vecMax.y)/2, 0, 1, 0, iResolution);
}

CTerrainLump::~CTerrainLump()
{
	if (m_iLowResTerrainVBO)
	{
		CRenderer::UnloadVertexDataFromGL(m_iLowResTerrainVBO);
		CRenderer::UnloadVertexDataFromGL(m_iLowResTerrainIBO);
	}
}

void CTerrainLump::Initialize()
{
	CLumpGenerationJob oJob;
	oJob.pManager = m_pManager;
	oJob.iLump = m_iLump;
	m_pManager->s_pLumpGenerator->AddJob(&oJob, sizeof(oJob));
}

void CTerrainLump::Think()
{
	CMutexLocker oLock = m_pManager->s_pLumpGenerator->GetLock();
	oLock.Lock();
	if (m_aiLowResDrop.size())
	{
		if (m_iLowResTerrainIBO)
			CRenderer::UnloadVertexDataFromGL(m_iLowResTerrainIBO);
		m_iLowResTerrainIBO = CRenderer::LoadIndexDataIntoGL(m_aiLowResDrop.size() * sizeof(unsigned int), m_aiLowResDrop.data());
		m_iLowResTerrainIBOSize = m_aiLowResDrop.size();
		m_aiLowResDrop.set_capacity(0);
	}
	oLock.Unlock();

	if (m_bGeneratingLowRes)
	{
		bool bUpdateTerrain = false;
		CMutexLocker oLock2 = m_pManager->s_pLumpGenerator->GetLock();
		if (oLock2.TryLock())
		{
			if (m_aflLowResDrop.size())
			{
				m_iLowResTerrainVBO = CRenderer::LoadVertexDataIntoGL(m_aflLowResDrop.size() * sizeof(float), m_aflLowResDrop.data());
				m_bGeneratingLowRes = false;
				m_aflLowResDrop.set_capacity(0);
				bUpdateTerrain = true;
			}
		}
		oLock2.Unlock();

		if (bUpdateTerrain)
			m_pManager->m_pPlanet->GetTerrain(m_iTerrain)->RebuildShell2Indices();
	}
}

void CTerrainLump::GenerateTerrain()
{
	if (m_iLowResTerrainVBO)
	{
		CRenderer::UnloadVertexDataFromGL(m_iLowResTerrainVBO);
		CRenderer::UnloadVertexDataFromGL(m_iLowResTerrainIBO);
	}

	m_iLowResTerrainVBO = 0;

	int iHighestLevel = m_pManager->m_pPlanet->ChunkDepth();
	int iLowestLevel = m_pManager->m_pPlanet->LumpDepth();

	int iResolution = iHighestLevel-iLowestLevel;

	CPlanetTerrain* pTerrain = m_pManager->m_pPlanet->GetTerrain(m_iTerrain);

	DoubleVector2D vecCoordCenter = (m_vecMin + m_vecMax)/2;
	m_vecLocalCenter = pTerrain->CoordToWorld(vecCoordCenter) + pTerrain->GenerateOffset(vecCoordCenter);

	tvector<CTerrainPoint> avecTerrain;
	size_t iRows = pTerrain->BuildTerrainArray(avecTerrain, m_mPlanetToLump, iResolution, m_vecMin, m_vecMax, m_vecLocalCenter);
	m_mLumpToPlanet = m_mPlanetToLump.InvertedRT();

	tvector<float> aflVerts;
	tvector<unsigned int> aiVerts;
	size_t iTriangles = CPlanetTerrain::BuildIndexedVerts(aflVerts, aiVerts, avecTerrain, iResolution, iRows);
	m_iLowResTerrainIBOSize = iTriangles*3;

	// Can't use the current GL context to create a VBO in this thread, so send the info to a drop where the main thread can pick it up.
	CMutexLocker oLock = m_pManager->s_pLumpGenerator->GetLock();
	oLock.Lock();
		TAssert(!m_aflLowResDrop.size());
		swap(m_aflLowResDrop, aflVerts);
		swap(m_aiLowResDrop, aiVerts);
}

// This isn't multithreaded but I'll leave the locks in there in case I want to do so later.
void CTerrainLump::RebuildIndices()
{
	TAssert(m_iLowResTerrainIBO);

	int iHighestLevel = m_pManager->m_pPlanet->ChunkDepth();
	int iLowestLevel = m_pManager->m_pPlanet->LumpDepth();

	int iResolution = iHighestLevel-iLowestLevel;

	tvector<CTerrainCoordinate> aiChunkCoordinates;
	for (size_t i = 0; i < m_pManager->m_pPlanet->GetChunkManager()->GetNumChunks(); i++)
	{
		CTerrainChunk* pChunk = m_pManager->m_pPlanet->GetChunkManager()->GetChunk(i);
		if (!pChunk)
			continue;

		if (pChunk->IsGeneratingLowRes())
			continue;

		if (pChunk->GetTerrain() != m_iTerrain)
			continue;

		if (m_pManager->GetLump(pChunk->GetLump()) != this)
			continue;

		CTerrainCoordinate& oCoord = aiChunkCoordinates.push_back();
		pChunk->GetCoordinates(oCoord.x, oCoord.y);
	}

	size_t iQuadsPerRow = (size_t)pow(2.0f, (float)iResolution);

	tvector<unsigned int> aiVerts;
	size_t iTriangles = CPlanetTerrain::BuildMeshIndices(aiVerts, aiChunkCoordinates, iResolution, iQuadsPerRow);

	// Can't use the current GL context to create a VBO in this thread, so send the info to a drop where the main thread can pick it up.
	CMutexLocker oLock = m_pManager->s_pLumpGenerator->GetLock();
	oLock.Lock();
	swap(m_aiLowResDrop, aiVerts);
}

void CTerrainLump::Render()
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

	CScalableVector vecLumpCenter;
	CScalableMatrix mLumpTransform;
	
	if (pCharacter->GetNearestPlanet() == pPlanet)
		vecLumpCenter = CScalableVector(m_vecLocalCenter, ePlanetScale) - vecCharacterOrigin;
	else
	{
		vecLumpCenter = mPlanetTransform.TransformVector(CScalableVector(m_vecLocalCenter, ePlanetScale) - vecCharacterOrigin);
		mLumpTransform = mPlanetTransform;
	}

	mLumpTransform.SetTranslation(vecLumpCenter);

	Matrix4x4 mLumpTransformMeters = mLumpTransform.GetUnits(eRenderScale);

	float flScale;
	if (eRenderScale == SCALE_RENDER)
		flScale = (float)CScalableFloat::ConvertUnits(1, ePlanetScale, SCALE_METER);
	else
		flScale = (float)CScalableFloat::ConvertUnits(1, ePlanetScale, eRenderScale);

	CScalableMatrix mPlanetToLocal = mPlanetTransform.InvertedRT();

	Vector vecStarLightPosition;
	if (pCharacter->GetNearestPlanet() == pPlanet)
		vecStarLightPosition = pStar->GameData().GetScalableRenderOrigin();
	else
		vecStarLightPosition = (mPlanetToLocal.TransformVector(pStar->GameData().GetScalableRenderOrigin())).GetUnits(eRenderScale);

	CRenderingContext r(GameServer()->GetRenderer(), true);

	r.ResetTransformations();

	r.Transform(mLumpTransformMeters);
	r.Scale(flScale, flScale, flScale);

	r.SetUniform("vecStarLightPosition", vecStarLightPosition);
	r.SetUniform("flScale", flScale);

	CScalableFloat flDistance = (pPlanet->GetGlobalOrigin() - pCharacter->GetGlobalOrigin()).Length() - pPlanet->GetRadius();
	float flAtmosphere = (float)RemapValClamped(flDistance, CScalableFloat(1.0f, SCALE_KILOMETER), pPlanet->GetAtmosphereThickness(), 1.0, 0.0);
	r.SetUniform("flAtmosphere", flAtmosphere);

	r.BeginRenderVertexArray(m_iLowResTerrainVBO);
	r.SetPositionBuffer(0u, 10*sizeof(float));
	r.SetNormalsBuffer(3*sizeof(float), 10*sizeof(float));
	r.SetTexCoordBuffer(6*sizeof(float), 10*sizeof(float), 0);
	r.SetTexCoordBuffer(8*sizeof(float), 10*sizeof(float), 1);
	r.EndRenderVertexArrayIndexed(m_iLowResTerrainIBO, m_iLowResTerrainIBOSize);
}

void CTerrainLump::GetCoordinates(unsigned short& x, unsigned short& y) const
{
	x = m_iX;
	y = m_iY;
}

size_t CTerrainLump::GetTerrain() const
{
	return m_iTerrain;
}
