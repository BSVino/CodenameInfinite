#include "planet_terrain.h"

#include <geometry.h>
#include <mtrand.h>

#include <renderer/renderingcontext.h>
#include <tinker/cvar.h>
#include <tinker/application.h>
#include <tinker/profiler.h>

#include "sp_game.h"
#include "sp_renderer.h"
#include "sp_playercharacter.h"
#include "planet.h"
#include "terrain_chunks.h"

CPlanetTerrain::CPlanetTerrain(class CPlanet* pPlanet, Vector vecDirection)
	: CTerrainQuadTree()
{
	m_pPlanet = pPlanet;
	m_vecDirection = vecDirection;
}

void CPlanetTerrain::Init()
{
	CBranchData oData;
	oData.bRender = true;
	oData.flScreenSize = 1;
	oData.flLastScreenUpdate = -1;
	oData.flLastPushPull = -1;
	oData.iRenderVectorsLastFrame = ~0;
	oData.iLocalCharacterDotLastFrame = ~0;
	oData.iShouldRenderLastFrame = ~0;
	oData.bCompletelyInsideFrustum = false;
	oData.iSplitSides = 0;
	CTerrainQuadTree::Init(this, oData);
}

CVar r_terrain_onequad("r_terrain_onequad", "off");

void CPlanetTerrain::Think()
{
	m_bOneQuad = r_terrain_onequad.GetBool();
	m_apRenderBranches.clear();
	m_iBuildsThisFrame = 0;
	m_pQuadTreeHead->m_oData.bCompletelyInsideFrustum = false;
	ThinkBranch(m_pQuadTreeHead);
}

CVar r_terrainbuildsperframe("r_terrainbuildsperframe", "10");
CVar r_freezeterrain("r_freezeterrain", "off");

void CPlanetTerrain::ThinkBranch(CTerrainQuadTreeBranch* pBranch)
{
	if (pBranch->m_pBranches[0])
	{
		for (size_t i = 0; i < (size_t)(m_bOneQuad?1:4); i++)
			pBranch->m_pBranches[i]->m_oData.bCompletelyInsideFrustum = pBranch->m_oData.bCompletelyInsideFrustum;
	}

	if (r_freezeterrain.GetBool())
	{
		if (pBranch->m_oData.bRender)
			ProcessBranchRendering(pBranch);
		else if (pBranch->m_pBranches[0])
		{
			for (size_t i = 0; i < (size_t)(m_bOneQuad?1:4); i++)
				ThinkBranch(pBranch->m_pBranches[i]);
		}

		return;
	}

	if (pBranch->m_oData.bRender)
	{
		// If I can render then maybe my kids can too?
		if (!pBranch->m_pBranches[0] && m_iBuildsThisFrame < r_terrainbuildsperframe.GetInt())
		{
			BuildBranch(pBranch);

			if (pBranch->m_pBranches[0])
				m_iBuildsThisFrame++;
		}

		bool bPush = ShouldPush(pBranch);

		if (bPush)
		{
			PushBranch(pBranch);

			if (pBranch->m_oData.bRender)
				ProcessBranchRendering(pBranch);
			else
			{
				for (size_t i = 0; i < (size_t)(m_bOneQuad?1:4); i++)
					ThinkBranch(pBranch->m_pBranches[i]);
			}
		}
		else
			ProcessBranchRendering(pBranch);

		return;
	}
	else if (!pBranch->m_pBranches[0])
	{
		TAssert(pBranch->m_iDepth > m_pPlanet->m_iChunkDepth);

		// If I can render then maybe my kids can too?
		if (m_iBuildsThisFrame < r_terrainbuildsperframe.GetInt())
		{
			BuildBranch(pBranch);

			if (pBranch->m_pBranches[0])
				m_iBuildsThisFrame++;
		}

		if (pBranch->m_pBranches[0])
		{
			for (size_t i = 0; i < (size_t)(m_bOneQuad?1:4); i++)
				ThinkBranch(pBranch->m_pBranches[i]);
		}

		return;
	}

	bool bPull = ShouldPull(pBranch);

	if (bPull)
	{
		PullBranch(pBranch);

		ProcessBranchRendering(pBranch);
	}
	else
	{
		// My kids should be rendering.
		for (size_t i = 0; i < (size_t)(m_bOneQuad?1:4); i++)
			ThinkBranch(pBranch->m_pBranches[i]);
	}
}

CVar r_terrainpushinterval("r_terrainpushinterval", "0.3");

bool CPlanetTerrain::ShouldPush(CTerrainQuadTreeBranch* pBranch)
{
	if (pBranch->m_oData.flLastPushPull >= 0 && GameServer()->GetGameTime() - pBranch->m_oData.flLastPushPull < r_terrainpushinterval.GetFloat())
		return false;

	if (pBranch->m_oData.flLastPushPull < 0)
		pBranch->m_oData.flLastPushPull = GameServer()->GetGameTime() + RandomFloat(0, 1);
	else
		pBranch->m_oData.flLastPushPull = GameServer()->GetGameTime();

	if (!pBranch->m_pBranches[0])
		return false;

	// See if we can push the render surface down to the next level.
	for (size_t i = 0; i < (size_t)(m_bOneQuad?1:4); i++)
	{
		if (ShouldRenderBranch(pBranch->m_pBranches[i]))
			return true;
	}

	return false;
}

CVar r_terrainpullinterval("r_terrainpullinterval", "0.1");

bool CPlanetTerrain::ShouldPull(CTerrainQuadTreeBranch* pBranch)
{
	if (pBranch->m_oData.flLastPushPull >= 0 && GameServer()->GetGameTime() - pBranch->m_oData.flLastPushPull < r_terrainpullinterval.GetFloat())
		return false;

	pBranch->m_oData.flLastPushPull = GameServer()->GetGameTime();

	if (!pBranch->m_pBranches[0])
		return false;

	// See if we can pull the render surface up from our kids.
	bool bAreKidsRenderingAndShouldnt = true;

	for (size_t i = 0; i < (size_t)(m_bOneQuad?1:4); i++)
	{
		bAreKidsRenderingAndShouldnt &= (pBranch->m_pBranches[i]->m_oData.bRender && !ShouldRenderBranch(pBranch->m_pBranches[i]));
		if (!bAreKidsRenderingAndShouldnt)
			break;
	}

	if (!bAreKidsRenderingAndShouldnt)
		return false;

	return true;
}

void CPlanetTerrain::BuildBranch(CTerrainQuadTreeBranch* pBranch, bool bForce)
{
	pBranch->BuildBranch(false, bForce);

	if (pBranch->m_pBranches[0])
	{
		if (pBranch->m_oData.bCompletelyInsideFrustum)
		{
			for (size_t i = 0; i < (size_t)(m_bOneQuad?1:4); i++)
				pBranch->m_pBranches[i]->m_oData.bCompletelyInsideFrustum = pBranch->m_oData.bCompletelyInsideFrustum;
		}

		// Push our already-generated offsets down to the kids. They won't regenerate these.
		pBranch->m_pBranches[0]->m_oData.vecOffset1 = pBranch->m_oData.vecOffset1;
		pBranch->m_pBranches[1]->m_oData.vecOffset2 = pBranch->m_oData.vecOffset2;
		pBranch->m_pBranches[2]->m_oData.vecOffset3 = pBranch->m_oData.vecOffset3;
		pBranch->m_pBranches[3]->m_oData.vecOffset4 = pBranch->m_oData.vecOffset4;

		pBranch->m_pBranches[0]->m_oData.vecOffset4 = pBranch->m_oData.vecOffsetCenter;
		pBranch->m_pBranches[1]->m_oData.vecOffset3 = pBranch->m_oData.vecOffsetCenter;
		pBranch->m_pBranches[2]->m_oData.vecOffset2 = pBranch->m_oData.vecOffsetCenter;
		pBranch->m_pBranches[3]->m_oData.vecOffset1 = pBranch->m_oData.vecOffsetCenter;
	}

	// If I create branches this frame, be sure to test right away if we should push them.
	pBranch->m_oData.flLastPushPull = -1;
}

CVar r_terrainresolution("r_terrainresolution", "1.0");

void CPlanetTerrain::PushBranch(CTerrainQuadTreeBranch* pBranch)
{
	if (!pBranch->m_pBranches[0])
		// Force building the branch, we want to guarantee it will get built because we need it to push to.
		BuildBranch(pBranch, true);

	// Don't push past chunk depth. Chunks handle things from there on out.
	if (pBranch->m_iDepth >= m_pPlanet->m_iChunkDepth)
		return;

	pBranch->m_oData.bRender = false;
	for (size_t i = 0; i < (size_t)(m_bOneQuad?1:4); i++)
		pBranch->m_pBranches[i]->m_oData.bRender = true;

	if (pBranch->m_pBranches[0]->m_iDepth >= m_pPlanet->m_iChunkDepth)
	{
		for (size_t i = 0; i < (size_t)(m_bOneQuad?1:4); i++)
			m_pPlanet->m_pTerrainChunkManager->AddChunk(pBranch->m_pBranches[i]);
	}

	CTerrainQuadTreeBranch* pNeighbor;

	// If I'm a higher level than my neighbor then push my neighbor to get him
	// down to my level. Keep all quads within one level of each other. This is
	// so that I can make sure a quad renders properly.

	pNeighbor = FindNeighbor(pBranch, QUADSIDE_LEFT);
	if (pNeighbor)
	{
		// If I'm pushing, tell my neighbor that he needs to split his opposite side.
		pNeighbor->m_oData.iSplitSides |= (1<<QUADSIDE_RIGHT);

		// If my neighbor's kids are rendering, I need to split.
		if (pNeighbor->m_pBranches[0] && (pNeighbor->m_pBranches[0]->m_oData.bRender || pNeighbor->m_pBranches[2]->m_oData.bRender))
			pBranch->m_oData.iSplitSides |= (1<<QUADSIDE_LEFT);

		if (pNeighbor->m_pParent && pNeighbor->m_pParent->m_oData.bRender)
			PushBranch(pNeighbor->m_pParent);
	}

	pNeighbor = FindNeighbor(pBranch, QUADSIDE_RIGHT);
	if (pNeighbor)
	{
		pNeighbor->m_oData.iSplitSides |= (1<<QUADSIDE_LEFT);

		if (pNeighbor->m_pBranches[0] && (pNeighbor->m_pBranches[1]->m_oData.bRender || pNeighbor->m_pBranches[3]->m_oData.bRender))
			pBranch->m_oData.iSplitSides |= (1<<QUADSIDE_RIGHT);

		if (pNeighbor->m_pParent && pNeighbor->m_pParent->m_oData.bRender)
			PushBranch(pNeighbor->m_pParent);
	}

	pNeighbor = FindNeighbor(pBranch, QUADSIDE_TOP);
	if (pNeighbor)
	{
		pNeighbor->m_oData.iSplitSides |= (1<<QUADSIDE_BOTTOM);

		if (pNeighbor->m_pBranches[0] && (pNeighbor->m_pBranches[0]->m_oData.bRender || pNeighbor->m_pBranches[1]->m_oData.bRender))
			pBranch->m_oData.iSplitSides |= (1<<QUADSIDE_TOP);

		if (pNeighbor->m_pParent && pNeighbor->m_pParent->m_oData.bRender)
			PushBranch(pNeighbor->m_pParent);
	}

	pNeighbor = FindNeighbor(pBranch, QUADSIDE_BOTTOM);
	if (pNeighbor)
	{
		pNeighbor->m_oData.iSplitSides |= (1<<QUADSIDE_TOP);

		if (pNeighbor->m_pBranches[0] && (pNeighbor->m_pBranches[2]->m_oData.bRender || pNeighbor->m_pBranches[3]->m_oData.bRender))
			pBranch->m_oData.iSplitSides |= (1<<QUADSIDE_BOTTOM);

		if (pNeighbor->m_pParent && pNeighbor->m_pParent->m_oData.bRender)
			PushBranch(pNeighbor->m_pParent);
	}
}

void CPlanetTerrain::PullBranch(CTerrainQuadTreeBranch* pBranch)
{
	TAssert(!pBranch->m_oData.bRender);
	TAssert(pBranch->m_pBranches[0]->m_oData.bRender);

	pBranch->m_oData.bRender = true;
	for (size_t i = 0; i < (size_t)(m_bOneQuad?1:4); i++)
		pBranch->m_pBranches[i]->m_oData.bRender = false;

	if (pBranch->m_pBranches[0]->m_iDepth == m_pPlanet->m_iChunkDepth)
	{
		for (size_t i = 0; i < (size_t)(m_bOneQuad?1:4); i++)
			m_pPlanet->m_pTerrainChunkManager->RemoveChunk(pBranch->m_pBranches[i]);
	}

	CTerrainQuadTreeBranch* pNeighbor;

	// Tell my neighbors that they no longer need to split their opposite sides.
	pNeighbor = FindNeighbor(pBranch, QUADSIDE_LEFT);
	if (pNeighbor)
	{
		pNeighbor->m_oData.iSplitSides &= ~(1<<QUADSIDE_RIGHT);

		if (pNeighbor->m_pBranches[0])
		{
			// If my neighbors are lower depth than I then I need to split on that side.
			if (pNeighbor->m_pBranches[1]->m_oData.bRender || pNeighbor->m_pBranches[3]->m_oData.bRender)
				pBranch->m_oData.iSplitSides |= (1<<QUADSIDE_LEFT);
		}

		if (pNeighbor->m_oData.bRender || (pNeighbor->m_pParent && pNeighbor->m_pParent->m_oData.bRender))
			pBranch->m_oData.iSplitSides &= ~(1<<QUADSIDE_LEFT);
	}

	pNeighbor = FindNeighbor(pBranch, QUADSIDE_RIGHT);
	if (pNeighbor)
	{
		pNeighbor->m_oData.iSplitSides &= ~(1<<QUADSIDE_LEFT);

		if (pNeighbor->m_pBranches[0])
		{
			if (pNeighbor->m_pBranches[0]->m_oData.bRender || pNeighbor->m_pBranches[2]->m_oData.bRender)
				pBranch->m_oData.iSplitSides |= (1<<QUADSIDE_RIGHT);
		}

		if (pNeighbor->m_oData.bRender || (pNeighbor->m_pParent && pNeighbor->m_pParent->m_oData.bRender))
			pBranch->m_oData.iSplitSides &= ~(1<<QUADSIDE_RIGHT);
	}

	pNeighbor = FindNeighbor(pBranch, QUADSIDE_TOP);
	if (pNeighbor)
	{
		pNeighbor->m_oData.iSplitSides &= ~(1<<QUADSIDE_BOTTOM);

		if (pNeighbor->m_pBranches[0])
		{
			if (pNeighbor->m_pBranches[2]->m_oData.bRender || pNeighbor->m_pBranches[3]->m_oData.bRender)
				pBranch->m_oData.iSplitSides |= (1<<QUADSIDE_TOP);
		}

		if (pNeighbor->m_oData.bRender || (pNeighbor->m_pParent && pNeighbor->m_pParent->m_oData.bRender))
			pBranch->m_oData.iSplitSides &= ~(1<<QUADSIDE_TOP);
	}

	pNeighbor = FindNeighbor(pBranch, QUADSIDE_BOTTOM);
	if (pNeighbor)
	{
		pNeighbor->m_oData.iSplitSides &= ~(1<<QUADSIDE_TOP);

		if (pNeighbor->m_pBranches[0])
		{
			if (pNeighbor->m_pBranches[0]->m_oData.bRender || pNeighbor->m_pBranches[1]->m_oData.bRender)
				pBranch->m_oData.iSplitSides |= (1<<QUADSIDE_BOTTOM);
		}

		if (pNeighbor->m_oData.bRender || (pNeighbor->m_pParent && pNeighbor->m_pParent->m_oData.bRender))
			pBranch->m_oData.iSplitSides &= ~(1<<QUADSIDE_BOTTOM);
	}
}

CVar r_terrainbackfacecull("r_terrainbackfacecull", "on");

void CPlanetTerrain::ProcessBranchRendering(CTerrainQuadTreeBranch* pBranch)
{
	TAssert(pBranch->m_oData.bRender);

	if (r_terrainbackfacecull.GetBool() && GetLocalCharacterDot(pBranch) >= 0.1f)
		return;

	// Let the chunks handle it.
	if (pBranch->m_iDepth >= m_pPlanet->m_iChunkDepth)
	{
		m_pPlanet->m_pTerrainChunkManager->ProcessChunkRendering(pBranch);
		return;
	}

	CalcRenderVectors(pBranch);
	UpdateScreenSize(pBranch);

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
			m_apRenderBranches[eScale].push_back(pBranch);

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

void CPlanetTerrain::Render(class CRenderingContext* c) const
{
	eastl::map<scale_t, eastl::vector<CTerrainQuadTreeBranch*> >::const_iterator it = m_apRenderBranches.find(SPGame()->GetSPRenderer()->GetRenderingScale());
	if (it == m_apRenderBranches.end())
		return;

	const eastl::vector<CTerrainQuadTreeBranch*>& aRenderBranches = it->second;

	for (size_t i = 0; i < aRenderBranches.size(); i++)
		RenderBranch(aRenderBranches[i], c);
}

CVar r_showquaddebugoutlines("r_showquaddebugoutlines", "off");
CVar r_showquadnormals("r_showquadnormals", "off");
CVar r_showquadoffsets("r_showquadoffsets", "off");
CVar r_showquadspheres("r_showquadspheres", "-1");

void CPlanetTerrain::RenderBranch(const CTerrainQuadTreeBranch* pBranch, class CRenderingContext* c) const
{
	TAssert(pBranch->m_oData.iRenderVectorsLastFrame == GameServer()->GetFrame());

	scale_t ePlanet = m_pPlanet->GetScale();
	scale_t eRender = SPGame()->GetSPRenderer()->GetRenderingScale();
	CScalableMatrix mPlanetTransform = m_pPlanet->GetGlobalTransform();
	CPlayerCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
	CScalableVector vecCharacterLocalOrigin = pCharacter->GetLocalOrigin();
	CScalableVector vecCharacterGlobalOrigin = pCharacter->GetGlobalOrigin();

	CScalableVector svecCenter;
	CScalableVector svec1;
	CScalableVector svec2;
	CScalableVector svec3;
	CScalableVector svec4;

	if (pCharacter->GetMoveParent() == m_pPlanet)
	{
		svecCenter = mPlanetTransform.TransformNoTranslate(CScalableVector(pBranch->m_oData.vecCenter, ePlanet) - vecCharacterLocalOrigin);
		svec1 = mPlanetTransform.TransformNoTranslate(CScalableVector(pBranch->m_oData.vec1, ePlanet) - vecCharacterLocalOrigin);
		svec2 = mPlanetTransform.TransformNoTranslate(CScalableVector(pBranch->m_oData.vec2, ePlanet) - vecCharacterLocalOrigin);
		svec3 = mPlanetTransform.TransformNoTranslate(CScalableVector(pBranch->m_oData.vec3, ePlanet) - vecCharacterLocalOrigin);
		svec4 = mPlanetTransform.TransformNoTranslate(CScalableVector(pBranch->m_oData.vec4, ePlanet) - vecCharacterLocalOrigin);
	}
	else
	{
		svecCenter = pBranch->m_oData.vecGlobalQuadCenter - vecCharacterGlobalOrigin;
		svec1 = mPlanetTransform * CScalableVector(pBranch->m_oData.vec1, ePlanet) - vecCharacterGlobalOrigin;
		svec2 = mPlanetTransform * CScalableVector(pBranch->m_oData.vec2, ePlanet) - vecCharacterGlobalOrigin;
		svec3 = mPlanetTransform * CScalableVector(pBranch->m_oData.vec3, ePlanet) - vecCharacterGlobalOrigin;
		svec4 = mPlanetTransform * CScalableVector(pBranch->m_oData.vec4, ePlanet) - vecCharacterGlobalOrigin;
	}

	Vector vecCenter = svecCenter.GetUnits(eRender);
	Vector vec1 = svec1.GetUnits(eRender);
	Vector vec2 = svec2.GetUnits(eRender);
	Vector vec3 = svec3.GetUnits(eRender);
	Vector vec4 = svec4.GetUnits(eRender);

	c->BeginRenderTriFan();
		c->TexCoord((pBranch->m_vecMin + pBranch->m_vecMax)/2, 0);
		c->TexCoord((pBranch->m_oData.vecDetailMin + pBranch->m_oData.vecDetailMax)/2, 1);
		c->Normal(pBranch->m_oData.vecCenterNormal);
		c->Vertex(vecCenter);

		c->TexCoord(pBranch->m_vecMin, 0);
		c->TexCoord(pBranch->m_oData.vecDetailMin, 1);
		c->Normal(pBranch->m_oData.vec1n);
		c->Vertex(vec1);

		if ((pBranch->m_oData.iSplitSides & (1<<QUADSIDE_TOP)) && pBranch->m_pBranches[0])
		{
			CScalableVector svecSplit;
			if (pCharacter->GetMoveParent() == m_pPlanet)
				svecSplit = mPlanetTransform.TransformNoTranslate(CScalableVector(pBranch->m_pBranches[0]->m_oData.vec2, ePlanet) - vecCharacterLocalOrigin);
			else
				svecSplit = mPlanetTransform * CScalableVector(pBranch->m_pBranches[0]->m_oData.vec2, ePlanet) - vecCharacterGlobalOrigin;
			Vector vecSplit = svecSplit.GetUnits(eRender);

			c->TexCoord(DoubleVector2D(pBranch->m_pBranches[0]->m_vecMax.x, pBranch->m_pBranches[0]->m_vecMin.y), 0);
			c->TexCoord(DoubleVector2D(pBranch->m_pBranches[0]->m_oData.vecDetailMax.x, pBranch->m_pBranches[0]->m_oData.vecDetailMin.y), 1);
			c->Normal(pBranch->m_pBranches[0]->m_oData.vec2n);
			c->Vertex(vecSplit);
		}

		c->TexCoord(DoubleVector2D(pBranch->m_vecMax.x, pBranch->m_vecMin.y), 0);
		c->TexCoord(DoubleVector2D(pBranch->m_oData.vecDetailMax.x, pBranch->m_oData.vecDetailMin.y), 1);
		c->Normal(pBranch->m_oData.vec2n);
		c->Vertex(vec2);

		if ((pBranch->m_oData.iSplitSides & (1<<QUADSIDE_RIGHT)) && pBranch->m_pBranches[1])
		{
			CScalableVector svecSplit;
			if (pCharacter->GetMoveParent() == m_pPlanet)
				svecSplit = mPlanetTransform.TransformNoTranslate(CScalableVector(pBranch->m_pBranches[1]->m_oData.vec4, ePlanet) - vecCharacterLocalOrigin);
			else
				svecSplit = mPlanetTransform * CScalableVector(pBranch->m_pBranches[1]->m_oData.vec4, ePlanet) - vecCharacterGlobalOrigin;
			Vector vecSplit = svecSplit.GetUnits(eRender);

			c->TexCoord(pBranch->m_pBranches[1]->m_vecMax, 0);
			c->TexCoord(DoubleVector2D(pBranch->m_pBranches[3]->m_oData.vecDetailMax.x, pBranch->m_pBranches[3]->m_oData.vecDetailMin.y), 1);
			c->Normal(pBranch->m_pBranches[1]->m_oData.vec4n);
			c->Vertex(vecSplit);
		}

		c->TexCoord(pBranch->m_vecMax, 0);
		c->TexCoord(pBranch->m_oData.vecDetailMax, 1);
		c->Normal(pBranch->m_oData.vec4n);
		c->Vertex(vec4);

		if ((pBranch->m_oData.iSplitSides & (1<<QUADSIDE_BOTTOM)) && pBranch->m_pBranches[3])
		{
			CScalableVector svecSplit;
			if (pCharacter->GetMoveParent() == m_pPlanet)
				svecSplit = mPlanetTransform.TransformNoTranslate(CScalableVector(pBranch->m_pBranches[3]->m_oData.vec3, ePlanet) - vecCharacterLocalOrigin);
			else
				svecSplit = mPlanetTransform * CScalableVector(pBranch->m_pBranches[3]->m_oData.vec3, ePlanet) - vecCharacterGlobalOrigin;
			Vector vecSplit = svecSplit.GetUnits(eRender);

			c->TexCoord(DoubleVector2D(pBranch->m_pBranches[3]->m_vecMin.x, pBranch->m_pBranches[3]->m_vecMax.y), 0);
			c->TexCoord(DoubleVector2D(pBranch->m_pBranches[3]->m_oData.vecDetailMin.x, pBranch->m_pBranches[3]->m_oData.vecDetailMax.y), 1);
			c->Normal(pBranch->m_pBranches[3]->m_oData.vec3n);
			c->Vertex(vecSplit);
		}

		c->TexCoord(DoubleVector2D(pBranch->m_vecMin.x, pBranch->m_vecMax.y), 0);
		c->TexCoord(DoubleVector2D(pBranch->m_oData.vecDetailMin.x, pBranch->m_oData.vecDetailMax.y), 1);
		c->Normal(pBranch->m_oData.vec3n);
		c->Vertex(vec3);

		if ((pBranch->m_oData.iSplitSides & (1<<QUADSIDE_LEFT)) && pBranch->m_pBranches[2])
		{
			CScalableVector svecSplit;
			if (pCharacter->GetMoveParent() == m_pPlanet)
				svecSplit = mPlanetTransform.TransformNoTranslate(CScalableVector(pBranch->m_pBranches[2]->m_oData.vec1, ePlanet) - vecCharacterLocalOrigin);
			else
				svecSplit = mPlanetTransform * CScalableVector(pBranch->m_pBranches[2]->m_oData.vec1, ePlanet) - vecCharacterGlobalOrigin;
			Vector vecSplit = svecSplit.GetUnits(eRender);

			c->TexCoord(pBranch->m_pBranches[2]->m_vecMin, 0);
			c->TexCoord(DoubleVector2D(pBranch->m_pBranches[0]->m_oData.vecDetailMin.x, pBranch->m_pBranches[0]->m_oData.vecDetailMax.y), 1);
			c->Normal(pBranch->m_pBranches[2]->m_oData.vec1n);
			c->Vertex(vecSplit);
		}

		c->TexCoord(pBranch->m_vecMin, 0);
		c->TexCoord(pBranch->m_oData.vecDetailMin, 1);
		c->Normal(pBranch->m_oData.vec1n);
		c->Vertex(vec1);
	c->EndRender();

	if (r_showquadspheres.GetInt() == pBranch->m_iDepth)
	{
		float flRadius = (float)pBranch->m_oData.flGlobalRadius.GetUnits(eRender);
		CRenderingContext c(GameServer()->GetRenderer());
		c.BindTexture(0);
		c.SetColor(Color(255, 255, 255));
		c.Translate(vecCenter);
		c.Scale(flRadius, flRadius, flRadius);
		c.RenderSphere();
	}

	if (r_showquaddebugoutlines.GetBool())
	{
		CRenderingContext c(GameServer()->GetRenderer());
		c.BindTexture(0);
		c.SetColor(Color(255, 255, 255));
		c.BeginRenderDebugLines();
		c.Vertex(vecCenter);
		c.Vertex(vec1);

		if ((pBranch->m_oData.iSplitSides & (1<<QUADSIDE_TOP)) && pBranch->m_pBranches[0])
		{
			CScalableVector svecSplit;
			if (pCharacter->GetMoveParent() == m_pPlanet)
				svecSplit = mPlanetTransform.TransformNoTranslate(CScalableVector(pBranch->m_pBranches[0]->m_oData.vec2, ePlanet) - vecCharacterLocalOrigin);
			else
				svecSplit = mPlanetTransform * CScalableVector(pBranch->m_pBranches[0]->m_oData.vec2, ePlanet) - vecCharacterGlobalOrigin;
			Vector vecSplit = svecSplit.GetUnits(eRender);
			c.Vertex(vecSplit);
		}

		c.Vertex(vec2);

		if ((pBranch->m_oData.iSplitSides & (1<<QUADSIDE_RIGHT)) && pBranch->m_pBranches[0])
		{
			CScalableVector svecSplit;
			if (pCharacter->GetMoveParent() == m_pPlanet)
				svecSplit = mPlanetTransform.TransformNoTranslate(CScalableVector(pBranch->m_pBranches[1]->m_oData.vec4, ePlanet) - vecCharacterLocalOrigin);
			else
				svecSplit = mPlanetTransform * CScalableVector(pBranch->m_pBranches[1]->m_oData.vec4, ePlanet) - vecCharacterGlobalOrigin;
			Vector vecSplit = svecSplit.GetUnits(eRender);
			c.Vertex(vecSplit);
		}

		c.Vertex(vec4);

		if ((pBranch->m_oData.iSplitSides & (1<<QUADSIDE_BOTTOM)) && pBranch->m_pBranches[0])
		{
			CScalableVector svecSplit;
			if (pCharacter->GetMoveParent() == m_pPlanet)
				svecSplit = mPlanetTransform.TransformNoTranslate(CScalableVector(pBranch->m_pBranches[3]->m_oData.vec3, ePlanet) - vecCharacterLocalOrigin);
			else
				svecSplit = mPlanetTransform * CScalableVector(pBranch->m_pBranches[3]->m_oData.vec3, ePlanet) - vecCharacterGlobalOrigin;
			Vector vecSplit = svecSplit.GetUnits(eRender);
			c.Vertex(vecSplit);
		}

		c.Vertex(vec3);

		if ((pBranch->m_oData.iSplitSides & (1<<QUADSIDE_LEFT)) && pBranch->m_pBranches[0])
		{
			CScalableVector svecSplit;
			if (pCharacter->GetMoveParent() == m_pPlanet)
				svecSplit = mPlanetTransform.TransformNoTranslate(CScalableVector(pBranch->m_pBranches[2]->m_oData.vec1, ePlanet) - vecCharacterLocalOrigin);
			else
				svecSplit = mPlanetTransform * CScalableVector(pBranch->m_pBranches[2]->m_oData.vec1, ePlanet) - vecCharacterGlobalOrigin;
			Vector vecSplit = svecSplit.GetUnits(eRender);
			c.Vertex(vecSplit);
		}

		c.EndRender();
	}

	if (r_showquadnormals.GetBool())
	{
		CRenderingContext c(GameServer()->GetRenderer());
		c.BindTexture(0);
		c.SetColor(Color(255, 255, 255));
		c.BeginRenderDebugLines();
		c.Vertex(vec1);
		c.Vertex(vec1 + mPlanetTransform.TransformNoTranslate(pBranch->m_oData.vec1n));
		c.EndRender();
	}

	if (r_showquadoffsets.GetBool())
	{
		CScalableVector vecScalableOffset = CScalableVector(pBranch->m_oData.vecOffset1, m_pPlanet->GetScale());
		Vector vecOffset = mPlanetTransform.TransformNoTranslate(vecScalableOffset).GetUnits(eRender);

		CRenderingContext c(GameServer()->GetRenderer());
		c.BindTexture(0);
		c.SetColor(Color(255, 255, 255));
		c.BeginRenderDebugLines();
		c.Vertex(vec1);
		c.Vertex(vec1 - vecOffset);
		c.EndRender();
	}
}

CVar r_terrainupdateinterval("r_terrainupdateinterval", "1");

void CPlanetTerrain::UpdateScreenSize(CTerrainQuadTreeBranch* pBranch)
{
	if (pBranch->m_oData.flLastScreenUpdate < 0 || GameServer()->GetGameTime() - pBranch->m_oData.flLastScreenUpdate > r_terrainupdateinterval.GetFloat())
	{
		CalcRenderVectors(pBranch);

		CSPRenderer* pRenderer = SPGame()->GetSPRenderer();

		Vector vecUp;
		Vector vecForward;
		GameServer()->GetRenderer()->GetCameraVectors(&vecForward, NULL, &vecUp);

		CScalableMatrix mPlanet = m_pPlanet->GetGlobalTransform();
		CPlayerCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
		CScalableVector vecCharacterOrigin = pCharacter->GetGlobalOrigin();

		CScalableVector vecQuadCenter = pBranch->m_oData.vecGlobalQuadCenter - vecCharacterOrigin;

		Vector vecScreen = pRenderer->ScreenPositionAtScale(m_pPlanet->GetScale(), vecQuadCenter.GetUnits(m_pPlanet->GetScale()));
		Vector vecTop = pRenderer->ScreenPositionAtScale(m_pPlanet->GetScale(), (vecQuadCenter + vecUp*pBranch->m_oData.flGlobalRadius).GetUnits(m_pPlanet->GetScale()));
		float flWidth = (vecTop - vecScreen).Length()*2;

		pBranch->m_oData.flScreenSize = flWidth;
		pBranch->m_oData.flLastScreenUpdate = GameServer()->GetGameTime();
	}
}

CVar r_minterrainsize("r_minterrainsize", "100");

CVar r_terrainperspectivescale("r_terrainperspectivescale", "on");
CVar r_terrainfrustumcull("r_terrainfrustumcull", "on");

CVar r_maxquadrenderdepth("r_maxquadrenderdepth", "-1");

bool CPlanetTerrain::ShouldRenderBranch(CTerrainQuadTreeBranch* pBranch)
{
	if (pBranch->m_oData.iShouldRenderLastFrame == GameServer()->GetFrame())
		return pBranch->m_oData.bShouldRender;

	pBranch->m_oData.iShouldRenderLastFrame = GameServer()->GetFrame();
	pBranch->m_oData.bShouldRender = false;

	InitRenderVectors(pBranch);

	if (pBranch->m_oData.flRadiusMeters < r_terrainresolution.GetFloat())
		return false;

	float flDot = GetLocalCharacterDot(pBranch);

	if (r_terrainbackfacecull.GetBool() && flDot >= 0.4f)
		return false;

	if (pBranch->m_iDepth > m_pPlanet->GetMinQuadRenderDepth())
	{
		float flScale = RemapValClamped(flDot, -1, 1, 0.1f, 1);

		if (!r_terrainperspectivescale.GetBool())
			flScale = 1;

		UpdateScreenSize(pBranch);

		if (pBranch->m_oData.flScreenSize*flScale < r_minterrainsize.GetFloat())
			return false;
	}

	if (r_maxquadrenderdepth.GetInt() >= 0 && pBranch->m_iDepth > r_maxquadrenderdepth.GetInt())
		return false;

	if (r_terrainfrustumcull.GetBool() && pBranch->m_iDepth > m_pPlanet->GetMinQuadRenderDepth())
	{
		if (!pBranch->m_oData.bCompletelyInsideFrustum)
		{
			CalcRenderVectors(pBranch);

			CPlayerCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
			CScalableMatrix mPlanet = m_pPlanet->GetGlobalTransform();
			CScalableVector vecPlanetCenter = pBranch->m_oData.vecGlobalQuadCenter - pCharacter->GetGlobalOrigin();

			Vector vecPlanetCenterUnscaled = vecPlanetCenter.GetUnits(m_pPlanet->GetScale());
			float flRadiusUnscaled = (float)CScalableFloat(pBranch->m_oData.flRadiusMeters, SCALE_METER).GetUnits(m_pPlanet->GetScale());

			if (!SPGame()->GetSPRenderer()->IsInFrustumAtScaleSidesOnly(m_pPlanet->GetScale(), vecPlanetCenterUnscaled, flRadiusUnscaled))
				return false;

			if (SPGame()->GetSPRenderer()->FrustumContainsAtScaleSidesOnly(m_pPlanet->GetScale(), vecPlanetCenterUnscaled, flRadiusUnscaled))
				pBranch->m_oData.bCompletelyInsideFrustum = true;
		}
	}

	pBranch->m_oData.bShouldRender = true;
	return true;
}

void CPlanetTerrain::InitRenderVectors(CTerrainQuadTreeBranch* pBranch)
{
	if (pBranch->m_oData.flRadiusMeters == 0)
	{
		if (pBranch->m_iDepth >= 20)
		{
			DoubleVector vecParentOffsetCenter = pBranch->m_pParent->m_oData.vecOffsetCenter;
			DoubleVector vecParentOffset1 = pBranch->m_pParent->m_oData.vecOffset1;
			DoubleVector vecParentOffset2 = pBranch->m_pParent->m_oData.vecOffset2;
			DoubleVector vecParentOffset3 = pBranch->m_pParent->m_oData.vecOffset3;
			DoubleVector vecParentOffset4 = pBranch->m_pParent->m_oData.vecOffset4;

			// xy
			if (pBranch->m_iParentIndex == 0)
			{
				pBranch->m_oData.vecOffset1 = vecParentOffset1;
				pBranch->m_oData.vecOffset2 = (vecParentOffset1 + vecParentOffset2)/2;
				pBranch->m_oData.vecOffset3 = (vecParentOffset1 + vecParentOffset3)/2;
				pBranch->m_oData.vecOffset4 = vecParentOffsetCenter;
			}

			// Xy
			else if (pBranch->m_iParentIndex == 1)
			{
				pBranch->m_oData.vecOffset1 = (vecParentOffset2 + vecParentOffset1)/2;
				pBranch->m_oData.vecOffset2 = vecParentOffset2;
				pBranch->m_oData.vecOffset3 = vecParentOffsetCenter;
				pBranch->m_oData.vecOffset4 = (vecParentOffset2 + vecParentOffset4)/2;
			}

			// xY
			else if (pBranch->m_iParentIndex == 2)
			{
				pBranch->m_oData.vecOffset1 = (vecParentOffset3 + vecParentOffset1)/2;
				pBranch->m_oData.vecOffset2 = vecParentOffsetCenter;
				pBranch->m_oData.vecOffset3 = vecParentOffset3;
				pBranch->m_oData.vecOffset4 = (vecParentOffset3 + vecParentOffset4)/2;
			}

			// XY
			else if (pBranch->m_iParentIndex == 3)
			{
				pBranch->m_oData.vecOffset1 = vecParentOffsetCenter;
				pBranch->m_oData.vecOffset2 = (vecParentOffset4 + vecParentOffset2)/2;
				pBranch->m_oData.vecOffset3 = (vecParentOffset4 + vecParentOffset3)/2;
				pBranch->m_oData.vecOffset4 = vecParentOffset4;
			}

			pBranch->m_oData.vecOffsetCenter = (pBranch->m_oData.vecOffset1 + pBranch->m_oData.vecOffset2 + pBranch->m_oData.vecOffset3 + pBranch->m_oData.vecOffset4)/4;
		}
		else if (pBranch->m_iDepth == 0)
		{
			pBranch->m_oData.vecOffsetCenter = GenerateOffset((pBranch->m_vecMin + pBranch->m_vecMax)/2);
			pBranch->m_oData.vecOffset1 = GenerateOffset(pBranch->m_vecMin);
			pBranch->m_oData.vecOffset2 = GenerateOffset(DoubleVector2D(pBranch->m_vecMax.x, pBranch->m_vecMin.y));
			pBranch->m_oData.vecOffset3 = GenerateOffset(DoubleVector2D(pBranch->m_vecMin.x, pBranch->m_vecMax.y));
			pBranch->m_oData.vecOffset4 = GenerateOffset(pBranch->m_vecMax);
		}
		else
		{
			pBranch->m_oData.vecOffsetCenter = GenerateOffset((pBranch->m_vecMin + pBranch->m_vecMax)/2);

			// Don't regenerate offsets that our parents gave us.
			if (pBranch->m_iParentIndex != 0 && pBranch->m_iParentIndex != 3)
			{
				pBranch->m_oData.vecOffset1 = GenerateOffset(pBranch->m_vecMin);
				pBranch->m_oData.vecOffset4 = GenerateOffset(pBranch->m_vecMax);
			}

			if (pBranch->m_iParentIndex != 1 && pBranch->m_iParentIndex != 2)
			{
				pBranch->m_oData.vecOffset2 = GenerateOffset(DoubleVector2D(pBranch->m_vecMax.x, pBranch->m_vecMin.y));
				pBranch->m_oData.vecOffset3 = GenerateOffset(DoubleVector2D(pBranch->m_vecMin.x, pBranch->m_vecMax.y));
			}
		}

		// Offsets are generated in planet space, add them right onto the quad position.
		DoubleVector vecCenter = m_pDataSource->QuadTreeToWorld(this, (pBranch->m_vecMin + pBranch->m_vecMax)/2) + pBranch->m_oData.vecOffsetCenter;
		DoubleVector vec1 = m_pDataSource->QuadTreeToWorld(this, pBranch->m_vecMin) + pBranch->m_oData.vecOffset1;
		DoubleVector vec2 = m_pDataSource->QuadTreeToWorld(this, DoubleVector2D(pBranch->m_vecMax.x, pBranch->m_vecMin.y)) + pBranch->m_oData.vecOffset2;
		DoubleVector vec3 = m_pDataSource->QuadTreeToWorld(this, DoubleVector2D(pBranch->m_vecMin.x, pBranch->m_vecMax.y)) + pBranch->m_oData.vecOffset3;
		DoubleVector vec4 = m_pDataSource->QuadTreeToWorld(this, pBranch->m_vecMax) + pBranch->m_oData.vecOffset4;

		pBranch->m_oData.vecCenter = vecCenter;
		pBranch->m_oData.vec1 = vec1;
		pBranch->m_oData.vec2 = vec2;
		pBranch->m_oData.vec3 = vec3;
		pBranch->m_oData.vec4 = vec4;

		pBranch->m_oData.vecCenterNormal = (vec1-vecCenter).Normalized().Cross((vec2-vecCenter).Normalized()).Normalized();
		pBranch->m_oData.vec1n = (vec2-vec1).Normalized().Cross((vec3-vec1).Normalized()).Normalized();
		pBranch->m_oData.vec2n = (vec4-vec2).Normalized().Cross((vec1-vec2).Normalized()).Normalized();
		pBranch->m_oData.vec3n = (vec1-vec3).Normalized().Cross((vec4-vec3).Normalized()).Normalized();
		pBranch->m_oData.vec4n = (vec3-vec4).Normalized().Cross((vec2-vec4).Normalized()).Normalized();

		CScalableVector vecQuadCenter(vecCenter, m_pPlanet->GetScale());

		CScalableVector vecQuadMax1(pBranch->m_oData.vec1, m_pPlanet->GetScale());
		CScalableFloat flRadius1 = (vecQuadCenter - vecQuadMax1).Length();
		CScalableVector vecQuadMax2(pBranch->m_oData.vec2, m_pPlanet->GetScale());
		CScalableFloat flRadius2 = (vecQuadCenter - vecQuadMax2).Length();
		CScalableVector vecQuadMax3(pBranch->m_oData.vec3, m_pPlanet->GetScale());
		CScalableFloat flRadius3 = (vecQuadCenter - vecQuadMax3).Length();
		CScalableVector vecQuadMax4(pBranch->m_oData.vec4, m_pPlanet->GetScale());
		CScalableFloat flRadius4 = (vecQuadCenter - vecQuadMax4).Length();

		CScalableFloat flRadius = flRadius1;
		if (flRadius2 > flRadius)
			flRadius = flRadius2;
		if (flRadius3 > flRadius)
			flRadius = flRadius3;
		if (flRadius4 > flRadius)
			flRadius = flRadius4;

		pBranch->m_oData.flGlobalRadius = flRadius;
		pBranch->m_oData.flRadiusMeters = (float)flRadius.GetUnits(SCALE_METER);

		pBranch->m_oData.vecDetailMin = DoubleVector2D(0, 0);
		pBranch->m_oData.vecDetailMax = DoubleVector2D(1, 1);

		// Count how many divisions it takes to get down to resolution level.
		float flRadiusMeters = pBranch->m_oData.flRadiusMeters;
		while (flRadiusMeters > r_terrainresolution.GetFloat())
		{
			flRadiusMeters /= 2;
			pBranch->m_oData.vecDetailMax = pBranch->m_oData.vecDetailMax*2;
		}
	}
}

void CPlanetTerrain::CalcRenderVectors(CTerrainQuadTreeBranch* pBranch)
{
	if (pBranch->m_oData.iRenderVectorsLastFrame == GameServer()->GetFrame())
		return;

	pBranch->m_oData.iRenderVectorsLastFrame = GameServer()->GetFrame();

	pBranch->m_oData.vecGlobalQuadCenter = m_pPlanet->GetGlobalTransform() * CScalableVector(pBranch->GetCenter(), m_pPlanet->GetScale());
}

float CPlanetTerrain::GetLocalCharacterDot(CTerrainQuadTreeBranch* pBranch)
{
	if (pBranch->m_oData.iLocalCharacterDotLastFrame == GameServer()->GetFrame())
		return pBranch->m_oData.flLocalCharacterDot;

	pBranch->m_oData.iLocalCharacterDotLastFrame = GameServer()->GetFrame();

	DoubleVector vecNormal = DoubleVector(pBranch->m_oData.vec1n + pBranch->m_oData.vec4n)/2;

	return pBranch->m_oData.flLocalCharacterDot = (float)(pBranch->GetCenter()-m_pPlanet->GetCharacterLocalOrigin()).Normalized().Dot(vecNormal);
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

DoubleVector2D CPlanetTerrain::WorldToQuadTree(const CTerrainQuadTree* pTree, const DoubleVector& v) const
{
	TAssert(pTree == (CTerrainQuadTree*)this);

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

DoubleVector CPlanetTerrain::QuadTreeToWorld(const CTerrainQuadTree* pTree, const DoubleVector2D& v) const
{
	TAssert(pTree == (CTerrainQuadTree*)this);

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

DoubleVector2D CPlanetTerrain::WorldToQuadTree(CTerrainQuadTree* pTree, const DoubleVector& v)
{
	return WorldToQuadTree((const CTerrainQuadTree*)pTree, v);
}

DoubleVector CPlanetTerrain::QuadTreeToWorld(CTerrainQuadTree* pTree, const DoubleVector2D& v)
{
	return QuadTreeToWorld((const CTerrainQuadTree*)pTree, v);
}

DoubleVector CPlanetTerrain::GetBranchCenter(CTerrainQuadTreeBranch* pBranch)
{
	InitRenderVectors(pBranch);

	return pBranch->m_oData.vecCenter;
}

bool CPlanetTerrain::ShouldBuildBranch(CTerrainQuadTreeBranch* pBranch, bool& bDelete)
{
	bDelete = false;

	InitRenderVectors(pBranch);

	// If it'll be below the allowed resolution, don't build it.
	if (pBranch->m_oData.flRadiusMeters/2 < r_terrainresolution.GetFloat())
		return false;

	return ShouldRenderBranch(pBranch);
}

bool CPlanetTerrain::IsLeaf(CTerrainQuadTreeBranch* pBranch)
{
	return pBranch->m_oData.bRender;
}

bool CPlanetTerrain::CollideLocal(bool bAccurate, const CScalableVector& v1, const CScalableVector& v2, CTraceResult& tr)
{
	InitRenderVectors(m_pQuadTreeHead);

	CTraceResult tr2;
	LineSegmentIntersectsSphere(v1, v2, CScalableVector(m_pQuadTreeHead->m_oData.vecCenter, m_pPlanet->GetScale()), m_pQuadTreeHead->m_oData.flGlobalRadius, tr2);
	if (tr2.bHit || tr2.bStartInside)
		return CollideLocalBranch(m_pQuadTreeHead, bAccurate, v1, v2, tr);

	return false;
}

bool CPlanetTerrain::CollideLocalBranch(CTerrainQuadTreeBranch* pBranch, bool bAccurate, const CScalableVector& v1, const CScalableVector& v2, CTraceResult& tr)
{
	DoubleVector vecNormal = DoubleVector(pBranch->m_oData.vec1n + pBranch->m_oData.vec4n)/2;
	float flDot = (float)(v2-v1).NormalizedVector().Dot(vecNormal.Normalized());

	if (flDot > 0.2f)
		return false;

	if (bAccurate && !pBranch->m_pBranches[0] && pBranch->m_oData.flRadiusMeters/2 >= r_terrainresolution.GetFloat())
		BuildBranch(pBranch, true);

	if (pBranch->m_pBranches[0] && pBranch->m_iDepth <= m_pPlanet->m_iChunkDepth)
	{
		bool bInSphere = false;
		for (size_t i = 0; i < 4; i++)
		{
			InitRenderVectors(pBranch->m_pBranches[i]);

			CTraceResult tr2;
			LineSegmentIntersectsSphere(v1, v2, CScalableVector(pBranch->m_pBranches[i]->m_oData.vecCenter, m_pPlanet->GetScale()), pBranch->m_pBranches[i]->m_oData.flGlobalRadius, tr2);

			if (tr2.bHit || tr2.bStartInside)
			{
				bInSphere = true;
				CollideLocalBranch(pBranch->m_pBranches[i], bAccurate, v1, v2, tr);
			}
		}

		if (tr.bHit)
			return true;

		if (!bInSphere)
			return false;

		// If we hit the sphere but didn't hit anything, fall through and test our quad just in case it would have fallen through the cracks.
	}

	CScalableVector vec1 = CScalableVector(pBranch->m_oData.vec1, m_pPlanet->GetScale());
	CScalableVector vec2 = CScalableVector(pBranch->m_oData.vec2, m_pPlanet->GetScale());
	CScalableVector vec3 = CScalableVector(pBranch->m_oData.vec3, m_pPlanet->GetScale());
	CScalableVector vec4 = CScalableVector(pBranch->m_oData.vec4, m_pPlanet->GetScale());

	if (LineSegmentIntersectsTriangle(v1, v2, vec1, vec2, vec3, tr))
		return true;
	if (LineSegmentIntersectsTriangle(v1, v2, vec2, vec4, vec3, tr))
		return true;

	return false;
}

bool CPlanetTerrain::Collide(bool bAccurate, const CScalableVector& v1, const CScalableVector& v2, CTraceResult& tr)
{
	InitRenderVectors(m_pQuadTreeHead);
	CalcRenderVectors(m_pQuadTreeHead);

	CTraceResult tr2;
	LineSegmentIntersectsSphere(v1, v2, m_pQuadTreeHead->m_oData.vecGlobalQuadCenter, m_pQuadTreeHead->m_oData.flGlobalRadius, tr2);
	if (tr2.bHit || tr2.bStartInside)
		return CollideBranch(m_pQuadTreeHead, bAccurate, v1, v2, tr);

	return false;
}

bool CPlanetTerrain::CollideBranch(CTerrainQuadTreeBranch* pBranch, bool bAccurate, const CScalableVector& v1, const CScalableVector& v2, CTraceResult& tr)
{
	if (bAccurate && !pBranch->m_pBranches[0] && pBranch->m_oData.flRadiusMeters/2 >= r_terrainresolution.GetFloat())
		BuildBranch(pBranch, true);

	if (pBranch->m_pBranches[0] && pBranch->m_iDepth <= m_pPlanet->m_iChunkDepth)
	{
		bool bInSphere = false;
		for (size_t i = 0; i < 4; i++)
		{
			InitRenderVectors(pBranch->m_pBranches[i]);
			CalcRenderVectors(pBranch->m_pBranches[i]);

			CTraceResult tr2;
			LineSegmentIntersectsSphere(v1, v2, pBranch->m_pBranches[i]->m_oData.vecGlobalQuadCenter, pBranch->m_pBranches[i]->m_oData.flGlobalRadius, tr2);
			if (tr2.bHit || tr2.bStartInside)
			{
				bInSphere = true;
				CollideBranch(pBranch->m_pBranches[i], bAccurate, v1, v2, tr);
			}
		}

		if (tr.bHit)
			return true;

		if (!bInSphere)
			return false;

		// If we hit the sphere but didn't hit anything, fall through and test our quad just in case it would have fallen through the cracks.
	}

	CScalableMatrix mPlanet = m_pPlanet->GetGlobalTransform();
	CScalableVector vec1 = mPlanet * CScalableVector(pBranch->m_oData.vec1, m_pPlanet->GetScale());
	CScalableVector vec2 = mPlanet * CScalableVector(pBranch->m_oData.vec2, m_pPlanet->GetScale());
	CScalableVector vec3 = mPlanet * CScalableVector(pBranch->m_oData.vec3, m_pPlanet->GetScale());
	CScalableVector vec4 = mPlanet * CScalableVector(pBranch->m_oData.vec4, m_pPlanet->GetScale());

	if (LineSegmentIntersectsTriangle(v1, v2, vec1, vec2, vec3, tr))
		return true;
	if (LineSegmentIntersectsTriangle(v1, v2, vec2, vec4, vec3, tr))
		return true;

	return false;
}

