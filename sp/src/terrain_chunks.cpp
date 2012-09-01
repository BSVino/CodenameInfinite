#include "terrain_chunks.h"

#include <octree.h>

#include <renderer/renderingcontext.h>
#include <tinker/cvar.h>

#include "sp_game.h"
#include "sp_playercharacter.h"
#include "sp_renderer.h"
#include "star.h"
#include "planet.h"
#include "planet_terrain.h"

CTerrainChunkManager::CTerrainChunkManager(CPlanet* pPlanet)
{
	m_pPlanet = pPlanet;
}

void CTerrainChunkManager::AddChunk()
{
/*	size_t iIndex = ~0;
	for (size_t i = 0; i < m_apChunks.size(); i++)
	{
		if (!m_apChunks[i])
		{
			iIndex = i;
			break;
		}
	}

	if (iIndex == ~0)
	{
		iIndex = m_apChunks.size();
		m_apChunks.push_back();
	}

	m_apChunks[iIndex] = new CTerrainChunk(this, pBranch);

	m_apBranchChunks[pBranch] = iIndex;*/
}

void CTerrainChunkManager::RemoveChunk()
{
/*	size_t i = FindChunk(pBranch);

	if (i == ~0)
	{
		TAssert(i != ~0);
		return;
	}

	delete m_apChunks[i];
	m_apChunks[i] = NULL;*/
}

size_t CTerrainChunkManager::FindChunk() const
{
/*	tmap<CTerrainQuadTreeBranch*, size_t>::const_iterator it = m_apBranchChunks.find(pBranch);

	if (it == m_apBranchChunks.end())
		return ~0;

	return it->second;*/
	return ~0;
}

CTerrainChunk* CTerrainChunkManager::GetChunk(size_t iChunk) const
{
	if (iChunk >= m_apChunks.size())
		return NULL;

	return m_apChunks[iChunk];
}

void CTerrainChunkManager::Think()
{
	m_apRenderChunks.clear();

	for (size_t i = 0; i < m_apChunks.size(); i++)
	{
		CTerrainChunk* pChunk = m_apChunks[i];
		if (!pChunk)
			continue;

		pChunk->Think();
	}
}

void CTerrainChunkManager::ProcessChunkRendering()
{
/*	size_t iChunk = FindChunk(pBranch);

	if (iChunk == ~0)
	{
		TAssert(iChunk != ~0);
		return;
	}

	CPlanetTerrain* pTree = static_cast<CPlanetTerrain*>(pBranch->m_pTree);
	pTree->CalcRenderVectors(pBranch);
	pTree->UpdateScreenSize(pBranch);

	CPlayerCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
	CSPRenderer* pRenderer = SPGame()->GetSPRenderer();

	CScalableVector vecDistanceToQuad = pBranch->m_oData.vecGlobalQuadCenter - pCharacter->GetGlobalOrigin();

	int iPushes = 0;
	for (size_t i = 0; i < SCALESTACK_SIZE; i++)
	{
		scale_t eScale = (scale_t)(i+1);

		float flGlobalRadius = (float)pBranch->m_oData.flGlobalRadius.GetUnits(eScale);
		if (flGlobalRadius > 10000)
			continue;

		if (pRenderer->IsInFrustumAtScale(eScale, vecDistanceToQuad.GetUnits(eScale), flGlobalRadius))
		{
			m_apRenderChunks[eScale].push_back(m_apChunks[iChunk]);

			if (++iPushes == 2)
				return;
		}
		else
		{
			// If we have one push and the subsequent wasn't in frustum then we're in only one scale and we don't need to do any more.
			if (iPushes == 1)
				return;
		}
	}*/
}

CVar r_markchunks("r_markchunks", "off");

void CTerrainChunkManager::Render()
{
	scale_t eScale = SPGame()->GetSPRenderer()->GetRenderingScale();

	tmap<scale_t, tvector<CTerrainChunk*> >::const_iterator it = m_apRenderChunks.find(eScale);
	if (it == m_apRenderChunks.end())
		return;

	const tvector<CTerrainChunk*>& aRenderChunks = it->second;

	CStar* pStar = SPGame()->GetSPRenderer()->GetClosestStar();
	CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
	CScalableMatrix mPlanetToLocal = m_pPlanet->GetGlobalTransform();
	mPlanetToLocal.InvertRT();
	Vector vecStarLightPosition = (mPlanetToLocal.TransformVector(pStar->GameData().GetScalableRenderOrigin())).GetUnits(eScale);

	CRenderingContext c(GameServer()->GetRenderer());

	c.UseMaterial("textures/earth.mat");

	c.SetUniform("bDetail", eScale <= SCALE_METER);
	c.SetUniform("vecStarLightPosition", vecStarLightPosition);
	c.SetUniform("eScale", eScale);

	if (r_markchunks.GetBool())
		c.SetColor(Color(255, 0, 0));

	for (size_t i = 0; i < aRenderChunks.size(); i++)
		aRenderChunks[i]->Render(&c);
}

CTerrainChunk::CTerrainChunk(CTerrainChunkManager* pManager)
{
	m_pManager = pManager;
	m_iDepth = m_iMaxDepth = 0;
}

CTerrainChunk::~CTerrainChunk()
{
}

void CTerrainChunk::Think()
{
/*	CPlayerCharacter* pLocalCharacter = SPGame()->GetLocalPlayerCharacter();

	CScalableVector vecBranchCenter = m_pManager->m_pPlanet->GetGlobalTransform() * CScalableVector(m_pBranch->GetCenter(), m_pManager->m_pPlanet->GetScale());
	vecBranchCenter -= pLocalCharacter->GetGlobalOrigin();

	CScalableFloat flDistance = vecBranchCenter.Length();

	double flDepth = RemapValClamped(flDistance, m_pBranch->m_oData.flGlobalRadius, CScalableFloat(1.5f, SCALE_KILOMETER), m_pManager->m_pPlanet->ChunkSize(), 0);
	m_iDepth = (int)flDepth;

	if (m_iDepth > m_iMaxDepth)
		BuildBranchToDepth();*/
}

void CTerrainChunk::Render(class CRenderingContext* c)
{
//	CPlanetTerrain* pTree = static_cast<CPlanetTerrain*>(m_pBranch->m_pTree);
//	pTree->RenderBranch(m_pBranch, c);
}
