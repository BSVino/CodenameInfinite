#include "planet_terrain.h"

#include <geometry.h>
#include <mtrand.h>

#include <renderer/renderingcontext.h>
#include <tinker/cvar.h>
#include <tinker/application.h>
#include <tinker/profiler.h>

#include "sp_game.h"
#include "sp_renderer.h"
#include "sp_character.h"
#include "planet.h"

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

void CPlanetTerrain::ThinkBranch(CTerrainQuadTreeBranch* pBranch)
{
	if (pBranch->m_pBranches[0])
	{
		for (size_t i = 0; i < (size_t)(m_bOneQuad?1:4); i++)
			pBranch->m_pBranches[i]->m_oData.bCompletelyInsideFrustum = pBranch->m_oData.bCompletelyInsideFrustum;
	}

	if (pBranch->m_oData.bRender)
	{
		// If I can render then maybe my kids can too?
		if (!pBranch->m_pBranches[0] && m_iBuildsThisFrame < r_terrainbuildsperframe.GetInt())
		{
			pBranch->BuildBranch(false);

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

				m_iBuildsThisFrame++;
			}

			// If I create branches this frame, be sure to test right away if we should push them.
			pBranch->m_oData.flLastPushPull = -1;
		}

		bool bPush = ShouldPush(pBranch);

		if (bPush)
		{
			pBranch->m_oData.bRender = false;
			for (size_t i = 0; i < (size_t)(m_bOneQuad?1:4); i++)
			{
				pBranch->m_pBranches[i]->m_oData.bRender = true;
				ThinkBranch(pBranch->m_pBranches[i]);
			}
		}
		else
			ProcessBranchRendering(pBranch);

		return;
	}

	TAssert(pBranch->m_pBranches[0]);

	bool bPull = ShouldPull(pBranch);

	if (bPull)
	{
		pBranch->m_oData.bRender = true;
		for (size_t i = 0; i < (size_t)(m_bOneQuad?1:4); i++)
			pBranch->m_pBranches[i]->m_oData.bRender = false;

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

	return bAreKidsRenderingAndShouldnt;
}

CVar r_terrainbackfacecull("r_terrainbackfacecull", "on");

void CPlanetTerrain::ProcessBranchRendering(CTerrainQuadTreeBranch* pBranch)
{
	TAssert(pBranch->m_oData.bRender);

	if (r_terrainbackfacecull.GetBool() && GetLocalCharacterDot(pBranch) >= 0.1f)
		return;

	CalcRenderVectors(pBranch);
	UpdateScreenSize(pBranch);

	CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
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

void CPlanetTerrain::RenderBranch(const CTerrainQuadTreeBranch* pBranch, class CRenderingContext* c) const
{
	TAssert(pBranch->m_oData.iRenderVectorsLastFrame == GameServer()->GetFrame());

	scale_t ePlanet = m_pPlanet->GetScale();
	scale_t eRender = SPGame()->GetSPRenderer()->GetRenderingScale();
	CScalableMatrix mPlanetTransform = m_pPlanet->GetGlobalTransform();
	CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();

	CScalableVector svec1;
	CScalableVector svec2;
	CScalableVector svec3;
	CScalableVector svec4;

	if (pCharacter->GetMoveParent() == m_pPlanet)
	{
		CScalableVector vecCharacterOrigin = pCharacter->GetLocalOrigin();

		svec1 = mPlanetTransform.TransformNoTranslate(CScalableVector(pBranch->m_oData.vec1, ePlanet) - vecCharacterOrigin);
		svec2 = mPlanetTransform.TransformNoTranslate(CScalableVector(pBranch->m_oData.vec2, ePlanet) - vecCharacterOrigin);
		svec3 = mPlanetTransform.TransformNoTranslate(CScalableVector(pBranch->m_oData.vec3, ePlanet) - vecCharacterOrigin);
		svec4 = mPlanetTransform.TransformNoTranslate(CScalableVector(pBranch->m_oData.vec4, ePlanet) - vecCharacterOrigin);
	}
	else
	{
		CScalableVector vecCharacterOrigin = pCharacter->GetGlobalOrigin();

		svec1 = mPlanetTransform * CScalableVector(pBranch->m_oData.vec1, ePlanet) - vecCharacterOrigin;
		svec2 = mPlanetTransform * CScalableVector(pBranch->m_oData.vec2, ePlanet) - vecCharacterOrigin;
		svec3 = mPlanetTransform * CScalableVector(pBranch->m_oData.vec3, ePlanet) - vecCharacterOrigin;
		svec4 = mPlanetTransform * CScalableVector(pBranch->m_oData.vec4, ePlanet) - vecCharacterOrigin;
	}

	Vector vec1 = svec1.GetUnits(eRender);
	Vector vec2 = svec2.GetUnits(eRender);
	Vector vec3 = svec3.GetUnits(eRender);
	Vector vec4 = svec4.GetUnits(eRender);

	c->BeginRenderQuads();
		c->TexCoord(pBranch->m_vecMin, 0);
		c->TexCoord(pBranch->m_oData.vecDetailMin, 1);
		c->Normal(pBranch->m_oData.vec1n);
		c->Vertex(vec1);

		c->TexCoord(DoubleVector2D(pBranch->m_vecMax.x, pBranch->m_vecMin.y), 0);
		c->TexCoord(DoubleVector2D(pBranch->m_oData.vecDetailMax.x, pBranch->m_oData.vecDetailMin.y), 1);
		c->Normal(pBranch->m_oData.vec2n);
		c->Vertex(vec2);

		c->TexCoord(pBranch->m_vecMax, 0);
		c->TexCoord(pBranch->m_oData.vecDetailMax, 1);
		c->Normal(pBranch->m_oData.vec4n);
		c->Vertex(vec4);

		c->TexCoord(DoubleVector2D(pBranch->m_vecMin.x, pBranch->m_vecMax.y), 0);
		c->TexCoord(DoubleVector2D(pBranch->m_oData.vecDetailMin.x, pBranch->m_oData.vecDetailMax.y), 1);
		c->Normal(pBranch->m_oData.vec3n);
		c->Vertex(vec3);
	c->EndRender();

	if (r_showquaddebugoutlines.GetBool())
	{
		CRenderingContext c(GameServer()->GetRenderer());
		c.BindTexture(0);
		c.SetColor(Color(255, 255, 255));
		c.BeginRenderDebugLines();
		c.Vertex(vec1);
		c.Vertex(vec2);
		c.Vertex(vec4);
		c.Vertex(vec3);
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
		CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
		CScalableVector vecCharacterOrigin = pCharacter->GetGlobalOrigin();

		CScalableVector vecQuadCenter = pBranch->m_oData.vecGlobalQuadCenter;
		CScalableVector vecQuadLocalMax = CScalableVector(pBranch->m_oData.vec4, m_pPlanet->GetScale());
		CScalableVector vecQuadMax = mPlanet * vecQuadLocalMax;
		CScalableFloat flRadius = (vecQuadCenter-vecQuadMax).Length();

		vecQuadCenter -= vecCharacterOrigin;

		Vector vecScreen = pRenderer->ScreenPositionAtScale(m_pPlanet->GetScale(), vecQuadCenter.GetUnits(m_pPlanet->GetScale()));
		Vector vecTop = pRenderer->ScreenPositionAtScale(m_pPlanet->GetScale(), (vecQuadCenter + vecUp*flRadius).GetUnits(m_pPlanet->GetScale()));
		float flWidth = (vecTop - vecScreen).Length()*2;

		pBranch->m_oData.flScreenSize = flWidth;
		pBranch->m_oData.flGlobalRadius = flRadius;
		pBranch->m_oData.flLastScreenUpdate = GameServer()->GetGameTime();
	}
}

CVar r_terrainresolution("r_terrainresolution", "1.0");
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

	if (r_terrainfrustumcull.GetBool())
	{
		if (!pBranch->m_oData.bCompletelyInsideFrustum)
		{
			CalcRenderVectors(pBranch);

			CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
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
				pBranch->m_oData.vecOffset4 = (vecParentOffset1 + vecParentOffset2 + vecParentOffset3 + vecParentOffset4)/4;
			}

			// Xy
			if (pBranch->m_iParentIndex == 1)
			{
				pBranch->m_oData.vecOffset1 = (vecParentOffset2 + vecParentOffset1)/2;
				pBranch->m_oData.vecOffset2 = vecParentOffset2;
				pBranch->m_oData.vecOffset3 = (vecParentOffset1 + vecParentOffset2 + vecParentOffset3 + vecParentOffset4)/4;
				pBranch->m_oData.vecOffset4 = (vecParentOffset2 + vecParentOffset4)/2;
			}

			// xY
			if (pBranch->m_iParentIndex == 2)
			{
				pBranch->m_oData.vecOffset1 = (vecParentOffset3 + vecParentOffset1)/2;
				pBranch->m_oData.vecOffset2 = (vecParentOffset1 + vecParentOffset2 + vecParentOffset3 + vecParentOffset4)/4;
				pBranch->m_oData.vecOffset3 = vecParentOffset3;
				pBranch->m_oData.vecOffset4 = (vecParentOffset3 + vecParentOffset4)/2;
			}

			// XY
			if (pBranch->m_iParentIndex == 3)
			{
				pBranch->m_oData.vecOffset1 = (vecParentOffset1 + vecParentOffset2 + vecParentOffset3 + vecParentOffset4)/4;
				pBranch->m_oData.vecOffset2 = (vecParentOffset4 + vecParentOffset2)/2;
				pBranch->m_oData.vecOffset3 = (vecParentOffset4 + vecParentOffset3)/2;
				pBranch->m_oData.vecOffset4 = vecParentOffset4;
			}
		}
		else if (pBranch->m_iDepth == 0)
		{
			pBranch->m_oData.vecOffset1 = GenerateOffset(pBranch->m_vecMin);
			pBranch->m_oData.vecOffset2 = GenerateOffset(DoubleVector2D(pBranch->m_vecMax.x, pBranch->m_vecMin.y));
			pBranch->m_oData.vecOffset3 = GenerateOffset(DoubleVector2D(pBranch->m_vecMin.x, pBranch->m_vecMax.y));
			pBranch->m_oData.vecOffset4 = GenerateOffset(pBranch->m_vecMax);
		}
		else
		{
			// Don't regenerate offsets that our parents gave us.
			if (pBranch->m_iParentIndex != 0)
				pBranch->m_oData.vecOffset1 = GenerateOffset(pBranch->m_vecMin);

			if (pBranch->m_iParentIndex != 1)
				pBranch->m_oData.vecOffset2 = GenerateOffset(DoubleVector2D(pBranch->m_vecMax.x, pBranch->m_vecMin.y));

			if (pBranch->m_iParentIndex != 2)
				pBranch->m_oData.vecOffset3 = GenerateOffset(DoubleVector2D(pBranch->m_vecMin.x, pBranch->m_vecMax.y));

			if (pBranch->m_iParentIndex != 3)
				pBranch->m_oData.vecOffset4 = GenerateOffset(pBranch->m_vecMax);
		}

		// Offsets are generated in planet space, add them right onto the quad position.
		DoubleVector vec1 = m_pDataSource->QuadTreeToWorld(this, pBranch->m_vecMin) + pBranch->m_oData.vecOffset1;
		DoubleVector vec2 = m_pDataSource->QuadTreeToWorld(this, DoubleVector2D(pBranch->m_vecMax.x, pBranch->m_vecMin.y)) + pBranch->m_oData.vecOffset2;
		DoubleVector vec3 = m_pDataSource->QuadTreeToWorld(this, DoubleVector2D(pBranch->m_vecMin.x, pBranch->m_vecMax.y)) + pBranch->m_oData.vecOffset3;
		DoubleVector vec4 = m_pDataSource->QuadTreeToWorld(this, pBranch->m_vecMax) + pBranch->m_oData.vecOffset4;

		pBranch->m_oData.vec1 = vec1;
		pBranch->m_oData.vec2 = vec2;
		pBranch->m_oData.vec3 = vec3;
		pBranch->m_oData.vec4 = vec4;

		pBranch->m_oData.vec1n = (vec2-vec1).Normalized().Cross((vec3-vec1).Normalized()).Normalized();
		pBranch->m_oData.vec2n = (vec4-vec2).Normalized().Cross((vec1-vec2).Normalized()).Normalized();
		pBranch->m_oData.vec3n = (vec1-vec3).Normalized().Cross((vec4-vec3).Normalized()).Normalized();
		pBranch->m_oData.vec4n = (vec3-vec4).Normalized().Cross((vec2-vec4).Normalized()).Normalized();

		DoubleVector vecCenter = (pBranch->m_oData.vec1 + pBranch->m_oData.vec2 + pBranch->m_oData.vec3 + pBranch->m_oData.vec4)/4;
		CScalableVector vecQuadCenter(vecCenter, m_pPlanet->GetScale());
		CScalableVector vecQuadMax(pBranch->m_oData.vec4, m_pPlanet->GetScale());

		CScalableFloat flRadius = (vecQuadCenter - vecQuadMax).Length();
		pBranch->m_oData.flRadiusMeters = (float)flRadius.GetUnits(SCALE_METER);

		pBranch->m_oData.vecDetailMin = DoubleVector2D(0, 0);
		pBranch->m_oData.vecDetailMax = DoubleVector2D(1, 1);

		// Count how many divisions it takes to get down to resolution level.
		float flRadiusMeters = pBranch->m_oData.flRadiusMeters;
		int iDivisions = 0;
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

	DoubleVector vecNormal = DoubleVector(pBranch->m_oData.vec1n + pBranch->m_oData.vec3n)/2;

	return pBranch->m_oData.flLocalCharacterDot = (float)(pBranch->GetCenter()-m_pPlanet->GetCharacterLocalOrigin()).Normalized().Dot(vecNormal);
}

DoubleVector CPlanetTerrain::GenerateOffset(const DoubleVector2D& vecCoordinate)
{
	DoubleVector vecOffset;
	double flSpaceFactor = 1.1/2;
	double flHeightFactor = 0.1;
	for (size_t i = 0; i < TERRAIN_NOISE_ARRAY_SIZE; i++)
	{
		double flXFactor = vecCoordinate.x * flSpaceFactor;
		double flYFactor = vecCoordinate.y * flSpaceFactor;
		double x = m_pPlanet->m_aNoiseArray[i][0].Noise(flXFactor, flYFactor) * flHeightFactor;
		double y = m_pPlanet->m_aNoiseArray[i][1].Noise(flXFactor, flYFactor) * flHeightFactor;
		double z = m_pPlanet->m_aNoiseArray[i][2].Noise(flXFactor, flYFactor) * flHeightFactor;

		vecOffset += DoubleVector(x, y, z);

		flSpaceFactor = flSpaceFactor*2;
		flHeightFactor = flHeightFactor/1.5;
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

	return (pBranch->m_oData.vec1 + pBranch->m_oData.vec2 + pBranch->m_oData.vec3 + pBranch->m_oData.vec4)/4;
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
