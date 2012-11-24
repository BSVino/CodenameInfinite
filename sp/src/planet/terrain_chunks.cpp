#include "terrain_chunks.h"

#include <parallelize.h>

#include <renderer/renderingcontext.h>
#include <tinker/cvar.h>

#include <physics/physics.h>

#include "entities/sp_game.h"
#include "entities/sp_playercharacter.h"
#include "sp_renderer.h"
#include "entities/star.h"
#include "planet/planet.h"
#include "planet/planet_terrain.h"
#include "planet/terrain_lumps.h"
#include "trees.h"

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

	m_iActiveChunks = 0;

	m_bHaveGroupCenter = false;
}

void CTerrainChunkManager::AddChunk(size_t iLump, const DoubleVector2D& vecChunkMin, const DoubleVector2D& vecChunkMax)
{
	CTerrainLump* pLump = m_pPlanet->GetLumpManager()->GetLump(iLump);
	TAssert(pLump);
	if (!pLump)
		return;

	size_t iIndex = ~0;
	for (size_t i = 0; i < m_apChunks.size(); i++)
	{
		CTerrainChunk* pChunk = m_apChunks[i];
		if (pChunk)
		{
			if (pChunk->m_iTerrain == pLump->m_iTerrain && pChunk->m_iLump == iLump && pChunk->m_vecMin == vecChunkMin && pChunk->m_vecMax == vecChunkMax)
				return;
		}
		else if (!pChunk)
			iIndex = i;
	}

	CMutexLocker oLock = s_pChunkGenerator->GetLock();
	oLock.Lock();

	if (iIndex == ~0)
	{
		iIndex = m_apChunks.size();
		m_apChunks.push_back();
	}

	m_apChunks[iIndex] = new CTerrainChunk(this, iIndex, pLump->m_iTerrain, iLump, vecChunkMin, vecChunkMax);
	m_apChunks[iIndex]->Initialize();
}

void CTerrainChunkManager::RemoveChunk(size_t iChunk)
{
	if (iChunk == ~0)
	{
		TAssert(iChunk != ~0);
		return;
	}

	CMutexLocker oLock = s_pChunkGenerator->GetLock();
	oLock.Lock();
	RemoveChunkNoLock(iChunk);
}

void CTerrainChunkManager::LumpRemoved(size_t iLump)
{
	for (size_t i = 0; i < m_apChunks.size(); i++)
	{
		if (!m_apChunks[i])
			continue;

		if (m_apChunks[i]->GetLump() == iLump)
			RemoveChunk(i);
	}
}

void CTerrainChunkManager::RemoveChunkNoLock(size_t iChunk)
{
	if (m_apChunks[iChunk] && (m_apChunks[iChunk]->m_bGeneratingTerrain || !m_apChunks[iChunk]->m_bGenerationDone))
		return;

	if (m_apChunks[iChunk]->HasPhysicsEntity())
	{
		TAssert(m_iActiveChunks);
		m_iActiveChunks--;
	}

	delete m_apChunks[iChunk];
	m_apChunks[iChunk] = NULL;

	if (!m_iActiveChunks)
	{
		m_bHaveGroupCenter = false;
		GamePhysics()->RemoveAllEntities();
	}
}

CTerrainChunk* CTerrainChunkManager::GetChunk(size_t iChunk) const
{
	if (iChunk >= m_apChunks.size())
		return NULL;

	return m_apChunks[iChunk];
}

CVar chunk_check("chunk_check", "0.3");
CVar chunk_distance("chunk_distance", "0.0007");

void CTerrainChunkManager::Think()
{
	CPlayerCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
	if (pCharacter->GetNearestPlanet() != m_pPlanet)
	{
		CMutexLocker oLock = s_pChunkGenerator->GetLock();
		if (oLock.TryLock())
		{
			size_t iChunksRemaining = 0;
			tvector<size_t> aiRebuildLumps;

			for (size_t i = 0; i < m_apChunks.size(); i++)
			{
				if (m_apChunks[i])
				{
					if (m_apChunks[i]->m_bGeneratingTerrain)
					{
						iChunksRemaining++;
						continue;
					}

					bool bAdd = true;
					for (size_t j = 0; j < aiRebuildLumps.size(); j++)
					{
						if (aiRebuildLumps[j] == m_apChunks[i]->GetLump())
						{
							bAdd = false;
							break;
						}
					}

					if (bAdd)
						aiRebuildLumps.push_back(m_apChunks[i]->GetLump());

					RemoveChunkNoLock(i);
				}
			}

			for (size_t i = 0; i < aiRebuildLumps.size(); i++)
			{
				if (m_pPlanet->GetLumpManager()->GetLump(aiRebuildLumps[i]))
					m_pPlanet->GetLumpManager()->GetLump(aiRebuildLumps[i])->RebuildIndices();
			}

			if (iChunksRemaining == 0)
				m_apChunks.set_capacity(0);
		}
		return;
	}

	if (GameServer()->GetGameTime() >= m_flNextChunkCheck)
		AddNearbyChunks();

	float flMaxChunkDistance = chunk_distance.GetFloat()*2;
	float flMaxChunkDistanceSqr = flMaxChunkDistance*flMaxChunkDistance;

	tvector<size_t> aiRebuildLumps;
	for (size_t i = 0; i < m_apChunks.size(); i++)
	{
		CTerrainChunk* pChunk = m_apChunks[i];
		if (!pChunk)
			continue;

		if (m_pPlanet->m_vecCharacterLocalOrigin.DistanceSqr(pChunk->m_aabbBounds.Center()) > flMaxChunkDistanceSqr)
		{
			bool bAdd = true;
			for (size_t j = 0; j < aiRebuildLumps.size(); j++)
			{
				if (aiRebuildLumps[j] == m_apChunks[i]->GetLump())
				{
					bAdd = false;
					break;
				}
			}

			if (bAdd)
				aiRebuildLumps.push_back(m_apChunks[i]->GetLump());

			RemoveChunk(i);
		}

		// If the chunk wasn't removed, let it think.
		pChunk = m_apChunks[i];
		if (!pChunk)
			continue;

		pChunk->Think();
	}

	for (size_t i = 0; i < aiRebuildLumps.size(); i++)
	{
		if (m_pPlanet->GetLumpManager()->GetLump(aiRebuildLumps[i]))
			m_pPlanet->GetLumpManager()->GetLump(aiRebuildLumps[i])->RebuildIndices();
	}
}

void CTerrainChunkManager::FindCenterChunk()
{
	if (m_bHaveGroupCenter || !m_iActiveChunks)
		return;

	CPlayerCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
	DoubleMatrix4x4 mLocalMeters;
	if (pCharacter->GetMoveParent() == pCharacter->GameData().GetPlanet())
		mLocalMeters = pCharacter->GetLocalTransform().GetMeters();
	else
		mLocalMeters = (m_pPlanet->GetGlobalToLocalTransform() * pCharacter->GetGlobalTransform()).GetMeters();
	DoubleVector vecLocalMeters = mLocalMeters.GetTranslation();

	double flNearest;
	size_t iNearestChunk = ~0;
	for (size_t i = 0; i < GetNumChunks(); i++)
	{
		CTerrainChunk* pChunk = GetChunk(i);

		if (!pChunk)
			continue;

		if (!pChunk->HasPhysicsEntity())
			continue;

		if (iNearestChunk == ~0)
		{
			iNearestChunk = i;
			flNearest = pChunk->GetLocalCenter().DistanceSqr(vecLocalMeters);
			continue;
		}

		double flDistance = pChunk->GetLocalCenter().DistanceSqr(vecLocalMeters);
		if (flDistance < flNearest)
		{
			flNearest = flDistance;
			iNearestChunk = i;
		}
	}

	if (iNearestChunk == ~0)
		return;

	CTerrainChunk* pChunk = GetChunk(iNearestChunk);

	m_mGroupToPlanet = pChunk->GetChunkToPlanet();
	m_mPlanetToGroup = pChunk->GetPlanetToChunk();

	m_bHaveGroupCenter = true;

	for (size_t i = 0; i < GameServer()->GetMaxEntities(); i++)
	{
		CBaseEntity* pEntity = CBaseEntity::GetEntity(i);
		if (!pEntity)
			continue;

		if (!pEntity->IsInPhysics())
			continue;

		if (pEntity->GameData().GetPlanet() != m_pPlanet)
			continue;

		collision_type_t eCollisionType = CT_STATIC_MESH;

		DoubleMatrix4x4 mEntityLocalMeters;
		if (pEntity->GetMoveParent() == pEntity->GameData().GetPlanet())
			mEntityLocalMeters = pEntity->GetLocalTransform().GetMeters();
		else
			mEntityLocalMeters = (m_pPlanet->GetGlobalToLocalTransform() * pEntity->GetGlobalTransform()).GetMeters();

		Matrix4x4 mChunkEntity = m_mPlanetToGroup * mEntityLocalMeters;

		// For the purposes of physics, stand stuff up straight.
		mChunkEntity.SetUpVector(Vector(0, 1, 0));
		mChunkEntity.SetRightVector(mChunkEntity.GetForwardVector().Cross(mChunkEntity.GetUpVector()).Normalized());
		mChunkEntity.SetForwardVector(mChunkEntity.GetUpVector().Cross(mChunkEntity.GetRightVector()).Normalized());

		CSPCharacter* pCharacter = dynamic_cast<CSPCharacter*>(pEntity);
		if (pCharacter)
			eCollisionType = CT_CHARACTER;

		pEntity->SetPhysicsTransform(mChunkEntity);

		if (!GamePhysics()->IsEntityAdded(pEntity))
			GamePhysics()->AddEntity(pEntity, eCollisionType);
	}
}

void CTerrainChunkManager::AddNearbyChunks()
{
	double flChunkDistance = chunk_distance.GetFloat();

	for (size_t i = 0; i < m_pPlanet->m_pLumpManager->GetNumLumps(); i++)
	{
		CTerrainLump* pLump = m_pPlanet->m_pLumpManager->GetLump(i);
		if (!pLump)
			continue;

		if (pLump->m_bGeneratingLowRes)
			continue;

		if (!pLump->m_bKDTreeAvailable)
			continue;

		tvector<CTerrainArea> avecAreas = pLump->FindNearbyAreas(m_pPlanet->m_vecCharacterLocalOrigin, (float)flChunkDistance);

		size_t iMaxAreas = std::min((size_t)2, avecAreas.size());
		for (size_t j = 0; j < iMaxAreas; j++)
			AddChunk(i, avecAreas[j].vecMin, avecAreas[j].vecMax);
	}

	m_flNextChunkCheck = GameServer()->GetGameTime() + chunk_check.GetFloat();
}

void CTerrainChunkManager::GenerateChunk(size_t iChunk)
{
	CMutexLocker oLock = s_pChunkGenerator->GetLock();
	oLock.Lock();
		TAssert(iChunk >= 0 && iChunk < m_apChunks.size());

		if (iChunk >= m_apChunks.size() || !m_apChunks[iChunk])
			return;

		m_apChunks[iChunk]->m_bGeneratingTerrain = true;
	oLock.Unlock();

	m_apChunks[iChunk]->GenerateTerrain();
}

CVar r_markchunks("r_markchunks", "off");
CVar r_markgroups("r_markgroups", "off");

void CTerrainChunkManager::Render()
{
	scale_t eRenderScale = SPGame()->GetSPRenderer()->GetRenderingScale();
	if (eRenderScale >= SCALE_MEGAMETER)
		return;

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

			c.UseProgram("model");
			c.SetDepthTest(false);
			c.RenderWireBox(AABB(vecMinWorld, vecMaxWorld));
		}
	}

	if (r_markgroups.GetBool() && m_bHaveGroupCenter && SPGame()->GetSPRenderer()->GetRenderingScale() == SCALE_METER)
	{
		CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
		CPlanet* pPlanet = m_pPlanet;
		CScalableVector vecCharacterOrigin = m_pPlanet->GetGlobalToLocalTransform() * pCharacter->GetGlobalOrigin();
		CScalableMatrix mPlanetTransform = pPlanet->GetGlobalTransform();

		CRenderingContext c(GameServer()->GetRenderer(), true);

		CScalableVector vecGroupCenter;
		CScalableMatrix mGroupTransform;

		TAssert(pCharacter->GetNearestPlanet() == pPlanet);
		vecGroupCenter = CScalableVector(m_mGroupToPlanet.GetTranslation(), SCALE_METER) - vecCharacterOrigin;

		mGroupTransform.SetTranslation(vecGroupCenter);

		Matrix4x4 mGroupTransformMeters = mGroupTransform.GetMeters();

		c.UseProgram("model");
		c.ResetTransformations();
		c.Transform(mGroupTransformMeters);
		c.SetDepthTest(false);
		c.RenderWireBox(AABB(Vector(-1, -1, -1), Vector(1, 1, 1)));
	}

	double flScale = CScalableFloat::ConvertUnits(1, m_pPlanet->GetScale(), eRenderScale);

	DoubleVector vecCharacterOrigin;
	if (SPGame()->GetLocalPlayerCharacter()->GetMoveParent() == m_pPlanet)
		vecCharacterOrigin = SPGame()->GetLocalPlayerCharacter()->GetLocalOrigin().GetUnits(eRenderScale);
	else
		vecCharacterOrigin = (m_pPlanet->GetGlobalToLocalTransform() * SPGame()->GetLocalPlayerCharacter()->GetGlobalOrigin()).GetUnits(eRenderScale);

	for (size_t i = 0; i < m_apChunks.size(); i++)
	{
		CTerrainChunk* pChunk = m_apChunks[i];
		if (!pChunk)
			continue;

		if (pChunk->IsGeneratingTerrain())
			continue;

		if (!SPGame()->GetSPRenderer()->IsInFrustumAtScale(eRenderScale, Vector(pChunk->GetLocalCenter()*flScale-vecCharacterOrigin), (float)(pChunk->GetRadius()*flScale)))
			continue;

		pChunk->Render();
	}
}

bool CTerrainChunkManager::IsGenerating() const
{
	for (size_t i = 0; i < m_apChunks.size(); i++)
	{
		if (!m_apChunks[i])
			continue;

		if (m_apChunks[i]->IsGeneratingTerrain())
			return true;
	}

	return !s_pChunkGenerator->AreAllJobsDone();
}

size_t CTerrainChunk::s_iChunks = 0;

CTerrainChunk::CTerrainChunk(CTerrainChunkManager* pManager, size_t iChunk, size_t iTerrain, size_t iLump, const DoubleVector2D& vecMin, const DoubleVector2D& vecMax)
{
	m_iParity = s_iChunks++;

	m_pManager = pManager;
	m_iChunk = iChunk;
	m_iTerrain = iTerrain;
	m_iLump = iLump;
	m_vecMin = vecMin;
	m_vecMax = vecMax;

	CTerrainLump* pLump = m_pManager->m_pPlanet->GetLumpManager()->GetLump(m_iLump);
	TAssert(pLump);

	CPlanetTerrain* pTerrain = m_pManager->m_pPlanet->GetTerrain(m_iTerrain);

	DoubleVector vecWorldMin = pTerrain->CoordToWorld(m_vecMin) + pTerrain->GenerateOffset(m_vecMin);
	DoubleVector vecWorldMax = pTerrain->CoordToWorld(m_vecMax) + pTerrain->GenerateOffset(m_vecMax);
	m_aabbBounds = TemplateAABB<double>(vecWorldMin, vecWorldMax);

	m_bGeneratingTerrain = false;
	m_bGenerationDone = false;

	m_bLoadIntoPhysics = false;
	m_iPhysicsMesh = ~0;
	m_iPhysicsEntity = ~0;

	size_t iResolution = (size_t)pow(2.0f, (float)m_pManager->m_pPlanet->ChunkDepth()-m_pManager->m_pPlanet->LumpDepth());
	m_iX = (size_t)RemapVal((vecMin.x+vecMax.x)/2, pLump->m_vecMin.x, pLump->m_vecMax.x, 0, iResolution);
	m_iY = (size_t)RemapVal((vecMin.y+vecMax.y)/2, pLump->m_vecMin.y, pLump->m_vecMax.y, 0, iResolution);
}

CTerrainChunk::~CTerrainChunk()
{
	m_pManager->m_pPlanet->GetTreeManager()->UnloadChunk(this);

	TAssert(m_bGenerationDone);

	if (m_iPhysicsEntity != ~0)
	{
		GamePhysics()->RemoveExtra(m_iPhysicsEntity);
		GamePhysics()->UnloadExtraCollisionMesh(m_iPhysicsMesh);
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
	if (m_bGeneratingTerrain)
	{
		bool bUpdateTerrain = false;
		CMutexLocker oLock = m_pManager->s_pChunkGenerator->GetLock();
		if (oLock.TryLock())
		{
			if (m_bLoadIntoPhysics)
			{
				m_pManager->m_iActiveChunks++;

				DoubleVector vecOrigin;
				if (m_pManager->m_bHaveGroupCenter)
				{
					vecOrigin = m_mChunkToPlanet * vecOrigin;
					vecOrigin = m_pManager->m_mPlanetToGroup * vecOrigin;
				}

				m_iPhysicsEntity = GamePhysics()->AddExtra(m_iPhysicsMesh, vecOrigin);

				m_bLoadIntoPhysics = false;
				if (!m_pManager->m_bHaveGroupCenter)
					m_pManager->FindCenterChunk();

				m_pManager->m_pPlanet->GetTreeManager()->LoadChunk(this);
			}

			if (m_aflVBODrop.size())
			{
				m_iTerrainVBO = CRenderer::LoadVertexDataIntoGL(m_aflVBODrop.size() * sizeof(float), m_aflVBODrop.data());
				m_aflVBODrop.set_capacity(0);
				bUpdateTerrain = true;
			}

			while (m_aLODDrops.size())
			{
				m_aiTerrainLODs[m_aLODDrops.back().x][m_aLODDrops.back().y].iLODIBO[m_aLODDrops.back().m_eType] = CRenderer::LoadIndexDataIntoGL(m_aLODDrops.back().m_aiDrop.size() * sizeof(unsigned int), m_aLODDrops.back().m_aiDrop.data());
				m_aiTerrainLODs[m_aLODDrops.back().x][m_aLODDrops.back().y].iLODIBOSize[m_aLODDrops.back().m_eType] = m_aLODDrops.back().m_iSize;
				m_aLODDrops.pop_back();
			}

			if (m_bGenerationDone)
			{
				bUpdateTerrain = true;
				m_bGeneratingTerrain = false;
			}
		}
		oLock.Unlock();

		if (bUpdateTerrain)
		{
			if (m_pManager->m_pPlanet->GetLumpManager()->GetLump(m_iLump))
				m_pManager->m_pPlanet->GetLumpManager()->GetLump(m_iLump)->RebuildIndices();
		}
	}
}

void CTerrainChunk::GenerateTerrain()
{
	for (size_t x = 0; x < CHUNK_TERRAIN_LODS; x++)
	{
		for (size_t y = 0; y < CHUNK_TERRAIN_LODS; y++)
			m_aiTerrainLODs[x][y].UnloadAll();
	}

	int iHighestLevel = m_pManager->m_pPlanet->MeterDepth();
	int iLowestLevel = m_pManager->m_pPlanet->ChunkDepth();

	int iResolution = iHighestLevel-iLowestLevel;

	CPlanetTerrain* pTerrain = m_pManager->m_pPlanet->GetTerrain(m_iTerrain);

	DoubleVector2D vecCoordCenter = (m_vecMin + m_vecMax)/2;
	m_vecLocalCenter = pTerrain->CoordToWorld(vecCoordCenter) + pTerrain->GenerateOffset(vecCoordCenter);

	tvector<CTerrainPoint> avecTerrain;
	size_t iRows = pTerrain->BuildTerrainArray(avecTerrain, m_mPlanetToChunk, iResolution, m_vecMin, m_vecMax, m_vecLocalCenter, true);
	m_mChunkToPlanet = m_mPlanetToChunk.InvertedRT();

	DoubleVector vecCorner1 = avecTerrain[0].vec3DPosition;
	DoubleVector vecCorner2 = avecTerrain[iRows-1].vec3DPosition;
	DoubleVector vecCorner3 = avecTerrain.back().vec3DPosition;
	DoubleVector vecCorner4 = avecTerrain[(iRows-1)*iRows-1].vec3DPosition;

	double flDistanceSqr1 = vecCorner1.LengthSqr();
	double flDistanceSqr2 = vecCorner2.LengthSqr();
	double flDistanceSqr3 = vecCorner3.LengthSqr();
	double flDistanceSqr4 = vecCorner4.LengthSqr();

	m_flRadius = sqrt(std::max(flDistanceSqr1, std::max(flDistanceSqr2, std::max(flDistanceSqr3, flDistanceSqr4))));

	// Generate trees before physics so that it's available when it wants to load tree physics in.
	m_pManager->m_pPlanet->GetTreeManager()->GenerateTrees(this);

	TAssert(!m_aflPhysicsVerts.size());
	m_aflPhysicsVerts.clear();
	m_aiPhysicsVerts.clear();
	size_t iTriangles = CPlanetTerrain::BuildIndexedPhysVerts(m_aflPhysicsVerts, m_aiPhysicsVerts, avecTerrain, iResolution-1, iRows);

	m_iPhysicsMesh = GamePhysics()->LoadExtraCollisionMesh(iTriangles, m_aiPhysicsVerts.data(), m_aflPhysicsVerts.size()/3, m_aflPhysicsVerts.data());

	m_bLoadIntoPhysics = true;

	tvector<float> aflVerts;
	iTriangles = CPlanetTerrain::BuildVertsForIndex(aflVerts, avecTerrain, iResolution, iRows, true);

	// Can't use the current GL context to create a VBO in this thread, so send the info to a drop where the main thread can pick it up.
	CMutexLocker oLock = m_pManager->s_pChunkGenerator->GetLock();
	oLock.Lock();
		TAssert(!m_aflVBODrop.size());
		swap(m_aflVBODrop, aflVerts);
	oLock.Unlock();

	tvector<unsigned int> aiVerts;
	for (size_t x = 0; x < CHUNK_TERRAIN_LODS; x++)
	{
		for (size_t y = 0; y < CHUNK_TERRAIN_LODS; y++)
		{
			size_t iX = x*iRows/CHUNK_TERRAIN_LODS;
			size_t iY = y*iRows/CHUNK_TERRAIN_LODS;

			size_t iX3 = (x+1)*iRows/CHUNK_TERRAIN_LODS-1;
			size_t iY3 = (y+1)*iRows/CHUNK_TERRAIN_LODS-1;

			size_t iX2 = (iX + iX3)/2;
			size_t iY2 = (iY + iY3)/2;

			DoubleVector vecCenter;
			vecCenter = (avecTerrain[(iX*iRows)+iY].vec3DPosition + avecTerrain[(iX2*iRows)+iY2].vec3DPosition + avecTerrain[(iX3*iRows)+iY3].vec3DPosition)/3;
			m_aiTerrainLODs[y][x].vecCenter = vecCenter + m_vecLocalCenter;

			for (size_t i = 0; i < LOD_TOTAL; i++)
			{
				aiVerts.clear();

				size_t iStep = (size_t)pow((float)2, (float)i);
				size_t iLODTriangles = CPlanetTerrain::BuildMeshIndices(aiVerts, tvector<CTerrainCoordinate>(), iX, iY, iStep, iRows/CHUNK_TERRAIN_LODS, iRows, false);

				CMutexLocker oLock = m_pManager->s_pChunkGenerator->GetLock();
				oLock.Lock();
					CTerrainLODDrop& oDrop = m_aLODDrops.push_back();
					oDrop.m_aiDrop = aiVerts;
					oDrop.x = x;
					oDrop.y = y;
					oDrop.m_eType = (lod_type)i;
					oDrop.m_iSize = iLODTriangles*3;
				oLock.Unlock();
			}
		}
	}

	m_bGenerationDone = true;
}

CVar chunk_lod_min_distance("chunk_lod_min_distance", "0.0");
CVar chunk_lod_max_distance("chunk_lod_max_distance", "0.00015");

void CTerrainChunk::Render()
{
	if (!m_iTerrainVBO)
		return;

	scale_t eRenderScale = SPGame()->GetSPRenderer()->GetRenderingScale();
	CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
	CPlanet* pPlanet = m_pManager->m_pPlanet;
	CStar* pStar = SPGame()->GetSPRenderer()->GetClosestStar();
	scale_t ePlanetScale = pPlanet->GetScale();
	CScalableVector vecCharacterOrigin = pCharacter->GetLocalOrigin();
	if (pCharacter->GetMoveParent() != pPlanet)
	{
		TAssert(pCharacter->GetMoveParent()->GetMoveParent() == pPlanet);
		vecCharacterOrigin = pCharacter->GetMoveParent()->GetLocalTransform() * pCharacter->GetLocalOrigin();
	}
	CScalableMatrix mPlanetTransform = pPlanet->GetGlobalTransform();

	CScalableVector vecChunkCenter;
	CScalableMatrix mChunkTransform;

	if (pCharacter->GetNearestPlanet() == pPlanet)
		vecChunkCenter = CScalableVector(m_vecLocalCenter, ePlanetScale) - vecCharacterOrigin;
	else
	{
		vecChunkCenter = mPlanetTransform.TransformVector(CScalableVector(m_vecLocalCenter, ePlanetScale) - vecCharacterOrigin);
		mChunkTransform = mPlanetTransform;
	}

	mChunkTransform.SetTranslation(vecChunkCenter);

	Matrix4x4 mChunkTransformMeters = mChunkTransform.GetUnits(eRenderScale);

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

	r.Transform(mChunkTransformMeters);
	r.Scale(flScale, flScale, flScale);

	r.SetUniform("bDetail", true);
	r.SetUniform("vecStarLightPosition", vecStarLightPosition);
	r.SetUniform("eScale", eRenderScale);
	r.SetUniform("flScale", flScale);
	r.SetUniform("bHasUp", true);
	r.SetUniform("vecUp", m_vecLocalCenter.Normalized());

	CScalableFloat flDistance = (pPlanet->GetGlobalOrigin() - pCharacter->GetGlobalOrigin()).Length() - pPlanet->GetRadius();
	float flAtmosphere = (float)RemapValClamped(flDistance, CScalableFloat(1.0f, SCALE_KILOMETER), pPlanet->GetAtmosphereThickness(), 1.0, 0.0);
	r.SetUniform("flAtmosphere", flAtmosphere);

	float flLODMinDistanceSqr = chunk_lod_min_distance.GetFloat()*chunk_lod_min_distance.GetFloat();
	float flLODMaxDistanceSqr = chunk_lod_max_distance.GetFloat()*chunk_lod_max_distance.GetFloat();

	// Todo: Can I frustum cull these individually?
	for (size_t x = 0; x < CHUNK_TERRAIN_LODS; x++)
	{
		for (size_t y = 0; y < CHUNK_TERRAIN_LODS; y++)
		{
			double flDistanceSqr = (m_aiTerrainLODs[x][y].vecCenter - m_pManager->m_pPlanet->GetCharacterLocalOrigin()).LengthSqr();
			float flLOD = RemapValClamped((float)flDistanceSqr, flLODMinDistanceSqr, flLODMaxDistanceSqr, 0, (float)LOD_TOTAL-1);
			lod_type eLOD = (lod_type)(int)flLOD;

			r.BeginRenderVertexArray(m_iTerrainVBO);
			r.SetPositionBuffer(0u, 10*sizeof(float));
			r.SetNormalsBuffer(3*sizeof(float), 10*sizeof(float));
			r.SetTexCoordBuffer(6*sizeof(float), 10*sizeof(float), 0);
			r.SetTexCoordBuffer(8*sizeof(float), 10*sizeof(float), 1);
			r.EndRenderVertexArrayIndexed(m_aiTerrainLODs[x][y].iLODIBO[eLOD], m_aiTerrainLODs[x][y].iLODIBOSize[eLOD]);
		}
	}
}

bool CTerrainChunk::IsExtraPhysicsEntGround(size_t iEnt) const
{
	if (iEnt == ~0)
		return false;

	return iEnt == m_iPhysicsEntity;
}

void CTerrainChunk::GetCoordinates(unsigned short& x, unsigned short& y) const
{
	x = m_iX;
	y = m_iY;
}

size_t CTerrainChunk::GetTerrain() const
{
	return m_iTerrain;
}

size_t CTerrainChunk::GetLump() const
{
	return m_iLump;
}
