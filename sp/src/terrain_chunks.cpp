#include "terrain_chunks.h"

#include <octree.h>

#include <raytracer/raytracer.h>

#include <renderer/renderingcontext.h>
#include <tinker/cvar.h>

#include "sp_game.h"
#include "sp_playercharacter.h"
#include "sp_renderer.h"
#include "star.h"
#include "planet.h"

CTerrainChunkManager::CTerrainChunkManager(CPlanet* pPlanet)
{
	m_pPlanet = pPlanet;
}

void CTerrainChunkManager::AddChunk(CTerrainQuadTreeBranch* pBranch)
{
	size_t iIndex = ~0;
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

	m_apChunks[iIndex] = new CTerrainChunk(pBranch);

	double flRadius = pBranch->m_oData.flGlobalRadius.GetUnits(m_pPlanet->GetScale());
	m_pPlanet->m_pOctree->AddObject(CChunkOrQuad(m_pPlanet, iIndex), TemplateAABB<double>(pBranch->GetCenter() - DoubleVector(flRadius, flRadius, flRadius), pBranch->GetCenter() + DoubleVector(flRadius, flRadius, flRadius)));

	m_apBranchChunks[pBranch] = iIndex;
}

void CTerrainChunkManager::RemoveChunk(CTerrainQuadTreeBranch* pBranch)
{
	size_t i = FindChunk(pBranch);

	if (i == ~0)
	{
		TAssert(i != ~0);
		return;
	}

	m_pPlanet->m_pOctree->RemoveObject(CChunkOrQuad(m_pPlanet, i));

	double flRadius = pBranch->m_oData.flGlobalRadius.GetUnits(m_pPlanet->GetScale());
	m_pPlanet->m_pOctree->AddObject(CChunkOrQuad(m_pPlanet, pBranch), TemplateAABB<double>(pBranch->GetCenter() - DoubleVector(flRadius, flRadius, flRadius), pBranch->GetCenter() + DoubleVector(flRadius, flRadius, flRadius)));

	delete m_apChunks[i];
	m_apChunks[i] = NULL;
}

size_t CTerrainChunkManager::FindChunk(CTerrainQuadTreeBranch* pBranch)
{
	eastl::map<CTerrainQuadTreeBranch*, size_t>::iterator it = m_apBranchChunks.find(pBranch);

	if (it == m_apBranchChunks.end())
		return ~0;

	return it->second;
}

CTerrainChunk* CTerrainChunkManager::GetChunk(size_t iChunk)
{
	if (iChunk >= m_apChunks.size())
		return NULL;

	return m_apChunks[iChunk];
}

void CTerrainChunkManager::ProcessChunkRendering(CTerrainQuadTreeBranch* pBranch)
{
	size_t iChunk = FindChunk(pBranch);

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
	}
}

CVar r_markchunks("r_markchunks", "off");

void CTerrainChunkManager::Render()
{
	scale_t eScale = SPGame()->GetSPRenderer()->GetRenderingScale();

	eastl::map<scale_t, eastl::vector<CTerrainChunk*> >::const_iterator it = m_apRenderChunks.find(eScale);
	if (it == m_apRenderChunks.end())
		return;

	const eastl::vector<CTerrainChunk*>& aRenderChunks = it->second;

	CStar* pStar = SPGame()->GetSPRenderer()->GetClosestStar();
	CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
	CScalableMatrix mPlanetToLocal = m_pPlanet->GetGlobalTransform();
	mPlanetToLocal.InvertTR();
	Vector vecStarLightPosition = (mPlanetToLocal.TransformNoTranslate(pStar->GameData().GetScalableRenderOrigin())).GetUnits(eScale);

	CRenderingContext c(GameServer()->GetRenderer());

	c.BindTexture("textures/planet.png", 0);
	if (eScale <= SCALE_METER)
		c.BindTexture("textures/grass.png", 1);

	c.UseProgram("planet");
	c.SetUniform("iDiffuse", 0);
	if (eScale <= SCALE_METER)
		c.SetUniform("iDetail", 1);
	c.SetUniform("bDetail", eScale <= SCALE_METER);
	c.SetUniform("vecStarLightPosition", vecStarLightPosition);
	c.SetUniform("eScale", eScale);

	if (r_markchunks.GetBool())
		c.SetColor(Color(255, 0, 0));

	for (size_t i = 0; i < aRenderChunks.size(); i++)
		aRenderChunks[i]->Render(&c);

	m_apRenderChunks.clear();
}

CTerrainChunk::CTerrainChunk(CTerrainQuadTreeBranch* pBranch)
{
	m_pBranch = pBranch;

	DoubleVector vecCenter = pBranch->GetCenter();

	m_pRaytracer = new raytrace::CRaytracer();
	m_pRaytracer->SetMaxDepth(7);
	m_pRaytracer->AddTriangle(pBranch->m_oData.vec1 - vecCenter, pBranch->m_oData.vec2 - vecCenter, pBranch->m_oData.vec3 - vecCenter);
	m_pRaytracer->AddTriangle(pBranch->m_oData.vec2 - vecCenter, pBranch->m_oData.vec3 - vecCenter, pBranch->m_oData.vec4 - vecCenter);
	m_pRaytracer->BuildTree();
}

CTerrainChunk::~CTerrainChunk()
{
	delete m_pRaytracer;
}

void CTerrainChunk::Render(class CRenderingContext* c)
{
	CPlanetTerrain* pTree = static_cast<CPlanetTerrain*>(m_pBranch->m_pTree);
	pTree->RenderBranch(m_pBranch, c);
}

bool CTerrainChunk::Raytrace(const DoubleVector& vecStart, const DoubleVector& vecEnd, CCollisionResult& tr)
{
	return m_pRaytracer->Raytrace(Vector(vecStart), Vector(vecEnd), tr);
}
