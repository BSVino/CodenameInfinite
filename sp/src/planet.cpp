#include "planet.h"

#include <geometry.h>
#include <octree.h>

#include <renderer/renderingcontext.h>
#include <tinker/cvar.h>
#include <tinker/application.h>
#include <tinker/profiler.h>
#include <renderer/shaders.h>

#include "sp_game.h"
#include "sp_renderer.h"
#include "sp_camera.h"
#include "sp_playercharacter.h"
#include "star.h"
#include "planet_terrain.h"
#include "terrain_chunks.h"

REGISTER_ENTITY(CPlanet);

NETVAR_TABLE_BEGIN(CPlanet);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CPlanet);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, size_t, m_iRandomSeed);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CScalableFloat, m_flRadius);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CScalableFloat, m_flAtmosphereThickness);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, float, m_flMinutesPerRevolution);
	SAVEDATA_OMIT(m_iMinQuadRenderDepth);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, bool, m_bOneSurface);
	SAVEDATA_DEFINE(CSaveData::DATA_STRING, tstring, m_sPlanetName);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, Color, m_clrAtmosphere);
	SAVEDATA_OMIT(m_pTerrainFd);
	SAVEDATA_OMIT(m_pTerrainBk);
	SAVEDATA_OMIT(m_pTerrainRt);
	SAVEDATA_OMIT(m_pTerrainLf);
	SAVEDATA_OMIT(m_pTerrainUp);
	SAVEDATA_OMIT(m_pTerrainDn);
	SAVEDATA_OMIT(m_aNoiseArray);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CPlanet);
INPUTS_TABLE_END();

DoubleVector CPlanet::s_vecCharacterLocalOrigin;

Vector g_vecTerrainDirections[] =
{
	Vector(1, 0, 0),
	Vector(-1, 0, 0),
	Vector(0, 0, 1),
	Vector(0, 0, -1),
	Vector(0, 1, 0),
	Vector(0, -1, 0),
};

CPlanet::CPlanet()
{
	m_iRandomSeed = 0;

	for (size_t i = 0; i < 6; i++)
	{
		m_pTerrain[i] = new CPlanetTerrain(this, g_vecTerrainDirections[i]);
		m_pTerrain[i]->Init();
	}

	m_pOctree = NULL;

	m_pTerrainChunkManager = new CTerrainChunkManager(this);
}

CPlanet::~CPlanet()
{
	delete m_pTerrainChunkManager;

	if (m_pOctree)
		delete m_pOctree;

	for (size_t i = 0; i < 6; i++)
		delete m_pTerrain[i];
}

void CPlanet::Precache()
{
	PrecacheTexture("textures/planet.png");
	PrecacheTexture("textures/grass.png");
}

void CPlanet::Spawn()
{
	// 1500km, a bit smaller than the moon
	SetRadius(CScalableFloat(1.5f, SCALE_MEGAMETER));
	// 20 km thick
	SetAtmosphereThickness(CScalableFloat(20.0f, SCALE_KILOMETER));
}

CVar r_planet_onesurface("r_planet_onesurface", "off");

CVar planet_rotscale("planet_rotscale", "1");

void CPlanet::Think()
{
	TPROF("CPlanet::Think");

	BaseClass::Think();

	SetLocalAngles(GetLocalAngles() + EAngle(0, 360, 0)*(GameServer()->GetFrameTime()/60/m_flMinutesPerRevolution*planet_rotscale.GetFloat()));

	m_bOneSurface = r_planet_onesurface.GetBool();
}

CVar r_minquadrenderdepth("r_minquadrenderdepth", "-1");

void CPlanet::RenderUpdate()
{
	TPROF("CPlanet::RenderUpdate");

	Vector vecUp;
	Vector vecForward;
	GameServer()->GetRenderer()->GetCameraVectors(&vecForward, NULL, &vecUp);

	CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
	if (pCharacter->GetMoveParent() == this)
	{
		s_vecCharacterLocalOrigin = pCharacter->GetLocalOrigin().GetUnits(GetScale());
	}
	else
	{
		CScalableVector vecCharacterOrigin = pCharacter->GetGlobalOrigin();

		// Transforming every quad to global coordinates in ShouldRenderBranch() is expensive.
		// Instead, transform the player to the planet's local once and do the math in local space.
		CScalableMatrix mPlanetGlobalToLocal = GetGlobalToLocalTransform();
		s_vecCharacterLocalOrigin = (mPlanetGlobalToLocal * vecCharacterOrigin).GetUnits(GetScale());
	}

	Vector vecOrigin = (GetGlobalOrigin() - pCharacter->GetGlobalOrigin()).GetUnits(GetScale());
	Vector vecOutside = vecOrigin + vecUp * (float)GetRenderRadius().GetUnits(GetScale());

	Vector vecScreen = SPGame()->GetSPRenderer()->ScreenPositionAtScale(GetScale(), vecOrigin);
	Vector vecTop = SPGame()->GetSPRenderer()->ScreenPositionAtScale(GetScale(), vecOutside);

	float flWidth = (vecTop - vecScreen).Length()*2;

	if (flWidth < 20)
		m_iMinQuadRenderDepth = 1;
	else if (flWidth < 50)
		m_iMinQuadRenderDepth = 2;
	else
		m_iMinQuadRenderDepth = 3;

	if (r_minquadrenderdepth.GetInt() >= 0)
		m_iMinQuadRenderDepth = r_minquadrenderdepth.GetInt();

	m_pTerrainChunkManager->Think();

	for (size_t i = 0; i < (size_t)(m_bOneSurface?1:6); i++)
		m_pTerrain[i]->Think();
}

bool CPlanet::ShouldRenderAtScale(scale_t eScale) const
{
	for (size_t i = 0; i < (size_t)(m_bOneSurface?1:6); i++)
		if (m_pTerrain[i]->m_apRenderBranches.find(eScale) != m_pTerrain[i]->m_apRenderBranches.end())
			return true;

	return false;
}

CScalableFloat CPlanet::GetRenderRadius() const
{
	return m_flRadius + GetAtmosphereThickness();
}

CVar r_colorcodescales("r_colorcodescales", "off");
CVar debug_showplanetcollision("debug_showplanetcollision", "off");
CVar r_planetoctree("r_planetoctree", "off");
CVar r_planetcollision("r_planetcollision", "off");
CVar r_planets("r_planets", "on");

void CPlanet::PostRender(bool bTransparent) const
{
	BaseClass::PostRender(bTransparent);

	if (bTransparent)
		return;

	TPROF("CPlanet::PostRender");

	scale_t eScale = SPGame()->GetSPRenderer()->GetRenderingScale();

	if (r_planetoctree.GetBool() && eScale == SCALE_MEGAMETER)
		Debug_RenderOctree(m_pOctree->GetHead());

	if (r_planetcollision.GetBool())
		Debug_RenderCollision(m_pOctree->GetHead());

	if (debug_showplanetcollision.GetBool())
	{
		CSPCharacter* pLocalCharacter = SPGame()->GetLocalPlayerCharacter();
		Vector vecForward = GameServer()->GetRenderer()->GetCameraVector();
		CScalableVector vecUp = pLocalCharacter->GetUpVector();
		CTraceResult tr;
		
		if (pLocalCharacter->GetMoveParent() == this)
		{
			// For the debug rendering code in the octree.
			CRenderingContext c(GameServer()->GetRenderer());

			CScalableMatrix mGlobal = GetGlobalTransform();
			mGlobal.SetTranslation(CScalableVector());
			c.Transform(mGlobal);

			c.Translate(-pLocalCharacter->GetLocalOrigin().GetUnits(SPGame()->GetSPRenderer()->GetRenderingScale()));

			CScalableMatrix mGlobalToLocal = GetGlobalToLocalTransform();
			vecUp = mGlobalToLocal.TransformNoTranslate(vecUp);
			vecForward = mGlobalToLocal.TransformNoTranslate(vecForward);
			CScalableVector vecCharacter = pLocalCharacter->GetLocalOrigin() + vecUp * pLocalCharacter->EyeHeight();
			// I would normally never use const_cast if this weren't a block of debug code.
			const_cast<CPlanet*>(this)->CollideLocal(const_cast<CPlanet*>(this), vecCharacter, vecCharacter + vecForward * CScalableFloat(100.0f, SCALE_GIGAMETER), tr);
			tr.vecHit = GetGlobalTransform() * tr.vecHit;
			tr.vecNormal = GetGlobalTransform().TransformNoTranslate(tr.vecNormal);
		}
		else
		{
			// For the debug rendering code in the octree.
			CRenderingContext c(GameServer()->GetRenderer());
			c.Translate(-pLocalCharacter->GetGlobalOrigin().GetUnits(SPGame()->GetSPRenderer()->GetRenderingScale()));
			c.Transform(GetGlobalTransform().GetUnits(SPGame()->GetSPRenderer()->GetRenderingScale()));

			CScalableVector vecCharacter = pLocalCharacter->GetGlobalOrigin() + vecUp * pLocalCharacter->EyeHeight();
			const_cast<CPlanet*>(this)->Collide(const_cast<CPlanet*>(this), vecCharacter, vecCharacter + vecForward * CScalableFloat(100.0f, SCALE_GIGAMETER), tr);
		}

		if (tr.bHit)
		{
			CRenderingContext c(GameServer()->GetRenderer());

			CScalableVector vecRender = tr.vecHit - pLocalCharacter->GetGlobalOrigin();
			c.Translate(vecRender.GetUnits(SPGame()->GetSPRenderer()->GetRenderingScale()));
			c.SetColor(Color(255, 255, 255));
			c.BeginRenderDebugLines();
			c.Vertex(tr.vecNormal*-10000);
			c.Vertex(tr.vecNormal*10000);
			c.EndRender();
			c.SetColor(Color(255, 0, 0));
			c.BeginRenderDebugLines();
			c.Vertex(Vector(-10000, 0, 0));
			c.Vertex(Vector(10000, 0, 0));
			c.EndRender();
			c.SetColor(Color(0, 255, 0));
			c.BeginRenderDebugLines();
			c.Vertex(Vector(0, -10000, 0));
			c.Vertex(Vector(0, 10000, 0));
			c.EndRender();
			c.SetColor(Color(0, 0, 255));
			c.BeginRenderDebugLines();
			c.Vertex(Vector(0, 0, -10000));
			c.Vertex(Vector(0, 0, 10000));
			c.EndRender();
			c.SetColor(Color(255, 255, 255));
			c.Scale(0.1f, 0.1f, 0.1f);
			c.RenderSphere();
		}
	}

	if (!r_planets.GetBool())
		return;

	CStar* pStar = SPGame()->GetSPRenderer()->GetClosestStar();
	CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
	CScalableMatrix mPlanetToLocal = GetGlobalTransform();
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

	if (r_colorcodescales.GetBool())
	{
		if (eScale == SCALE_KILOMETER)
			c.SetColor(Color(0, 0, 255));
		else if (eScale == SCALE_MEGAMETER)
			c.SetColor(Color(0, 255, 0));
		else if (eScale == SCALE_GIGAMETER)
			c.SetColor(Color(255, 0, 0));
		else if (eScale == SCALE_METER)
			c.SetColor(Color(255, 255, 0));
		else
			c.SetColor(Color(255, 0, 255));
	}
	else
		c.SetColor(Color(255, 255, 255));

	for (size_t i = 0; i < (size_t)(m_bOneSurface?1:6); i++)
		m_pTerrain[i]->Render(&c);

	m_pTerrainChunkManager->Render();
}

bool CPlanet::CollideLocal(const CBaseEntity* pWith, const TVector& v1, const TVector& v2, CTraceResult& tr)
{
	return CollideLocalAccurate(pWith, false, v1, v2, tr);
}

bool CPlanet::CollideLocalAccurate(const CBaseEntity* pWith, bool bAccurate, const TVector& v1, const TVector& v2, CTraceResult& tr)
{
	if (v1.Length() > CScalableFloat(500.0f, SCALE_MEGAMETER))
		return false;

	if (tr.bHit && tr.flFraction == 0)
		return false;

	if (!ShouldCollide())
		return false;

	if (GetBoundingRadius() == TFloat(0))
		return false;

/*	if (v1 == v2)
	{
		vecPoint = v1;
		TFloat flLength = v1.Length();
		vecNormal = v1/flLength;

		bool bLess = flLength < GetBoundingRadius();
		if (bLess)
			vecPoint += vecNormal * ((GetBoundingRadius()-flLength) + TFloat(0.0001f));

		return bLess;
	}*/

	CCollisionResult cr;
	m_pOctree->Raytrace(v1.GetUnits(GetScale()), v2.GetUnits(GetScale()), cr);
	tr.bHit = cr.bHit;
	tr.bStartInside = cr.bStartInside;
	tr.flFraction = cr.flFraction;
	tr.pHit = this;
	tr.vecHit = CScalableVector(cr.vecHit, GetScale());
	tr.vecNormal = cr.vecNormal;

	return tr.bHit;
}

bool CPlanet::Collide(const CBaseEntity* pWith, const TVector& v1, const TVector& v2, CTraceResult& tr)
{
	return CollideAccurate(pWith, false, v1, v2, tr);
}

bool CPlanet::CollideAccurate(const CBaseEntity* pWith, bool bAccurate, const TVector& v1, const TVector& v2, CTraceResult& tr)
{
	if ((v1 - GetGlobalOrigin()).Length() > CScalableFloat(500.0f, SCALE_MEGAMETER))
		return false;

	if (tr.bHit && tr.flFraction == 0)
		return false;

	if (!ShouldCollide())
		return false;

	if (GetBoundingRadius() == TFloat(0))
		return false;

/*	if (v1 == v2)
	{
		vecPoint = v1;
		TFloat flLength = v1.Length();
		vecNormal = v1/flLength;

		bool bLess = flLength < GetBoundingRadius();
		if (bLess)
			vecPoint += vecNormal * ((GetBoundingRadius()-flLength) + TFloat(0.0001f));

		return bLess;
	}*/

	CScalableMatrix mGlobalToLocal = GetGlobalToLocalTransform();
	CScalableVector vecLocal1, vecLocal2;
	vecLocal1 = (mGlobalToLocal * v1);
	vecLocal2 = (mGlobalToLocal * v2);
	CCollisionResult cr;
	m_pOctree->Raytrace(vecLocal1.GetUnits(GetScale()), vecLocal2.GetUnits(GetScale()), cr);

	tr.bHit = cr.bHit;
	tr.bStartInside = cr.bStartInside;
	tr.flFraction = cr.flFraction;
	tr.pHit = this;
	tr.vecHit = GetGlobalTransform() * CScalableVector(cr.vecHit, GetScale());
	tr.vecNormal = GetGlobalTransform().TransformNoTranslate(CScalableVector(cr.vecNormal, SCALE_METER));

	return tr.bHit;
}

void CPlanet::SetRandomSeed(size_t iSeed)
{
	m_iRandomSeed = iSeed;

	for (size_t i = 0; i < TERRAIN_NOISE_ARRAY_SIZE; i++)
	{
		for (size_t j = 0; j < 3; j++)
			m_aNoiseArray[i][j].Init(iSeed+i*3+j);
	}
}

void CPlanet::SetRadius(const CScalableFloat& flRadius)
{
	m_flRadius = flRadius;

	if (m_pOctree)
		delete m_pOctree;

	m_pOctree = new COctree<CChunkOrQuad, double>((flRadius*2.0f).GetUnits(GetScale()));

	m_pTerrain[0]->m_pQuadTreeHead->m_oData.flRadiusMeters = 0;
	m_pTerrain[0]->InitRenderVectors(m_pTerrain[0]->m_pQuadTreeHead);
	int iMeterDepth = (int)(log(m_pTerrain[0]->m_pQuadTreeHead->m_oData.flRadiusMeters)/log(2.0f));
	m_iChunkDepth = iMeterDepth - 11;
}

CScalableFloat CPlanet::GetCloseOrbit()
{
	// For Earth values this resolves to about 600km above the ground, or about twice the altitude of the ISS.
	return GetRadius()/10.0f;
}


void Planet_RebuildTerrain(class CCommand* pCommand, eastl::vector<tstring>& asTokens, const tstring& sCommand)
{
	CPlayerCharacter* pLocalPlayer = SPGame()->GetLocalPlayerCharacter();
	CPlanet* pNearestPlanet = pLocalPlayer->GetNearestPlanet(FINDPLANET_ANY);

	pNearestPlanet->Debug_RebuildTerrain();
}

CCommand planet_terrainrebuild("planet_terrainrebuild", Planet_RebuildTerrain);

void CPlanet::Debug_RebuildTerrain()
{
	for (size_t i = 0; i < 6; i++)
		delete m_pTerrain[i];

	for (size_t i = 0; i < 6; i++)
	{
		m_pTerrain[i] = new CPlanetTerrain(this, g_vecTerrainDirections[i]);
		m_pTerrain[i]->Init();
	}
}

void CPlanet::Debug_RenderOctree(const COctreeBranch<CChunkOrQuad, double>* pBranch) const
{
	if (!pBranch)
		return;

	if (r_planetoctree.GetInt() >= 0 && pBranch->m_iDepth > r_planetoctree.GetInt())
		return;

	if (pBranch->m_pBranches[0])
	{
		for (size_t i = 0; i < 8; i++)
			Debug_RenderOctree(pBranch->m_pBranches[i]);
		return;
	}

	if (r_planetoctree.GetInt() >= 0 && pBranch->m_iDepth != r_planetoctree.GetInt())
		return;

	if (!pBranch->m_aObjects.size())
		return;

	Matrix4x4 mPlanetTransform = GetGlobalTransform().GetUnits(GetScale());
	mPlanetTransform.SetTranslation(Vector());

	CRenderingContext c(GameServer()->GetRenderer());

	double flMinX = pBranch->m_oBounds.m_vecMins.x;
	double flMinY = pBranch->m_oBounds.m_vecMins.y;
	double flMinZ = pBranch->m_oBounds.m_vecMins.z;
	double flMaxX = pBranch->m_oBounds.m_vecMaxs.x;
	double flMaxY = pBranch->m_oBounds.m_vecMaxs.y;
	double flMaxZ = pBranch->m_oBounds.m_vecMaxs.z;

	DoubleVector vecxyz = mPlanetTransform * DoubleVector(flMinX, flMinY, flMinZ);
	DoubleVector vecXyz = mPlanetTransform * DoubleVector(flMaxX, flMinY, flMinZ);
	DoubleVector vecxYz = mPlanetTransform * DoubleVector(flMinX, flMaxY, flMinZ);
	DoubleVector vecXYz = mPlanetTransform * DoubleVector(flMaxX, flMaxY, flMinZ);
	DoubleVector vecxyZ = mPlanetTransform * DoubleVector(flMinX, flMinY, flMaxZ);
	DoubleVector vecXyZ = mPlanetTransform * DoubleVector(flMaxX, flMinY, flMaxZ);
	DoubleVector vecxYZ = mPlanetTransform * DoubleVector(flMinX, flMaxY, flMaxZ);
	DoubleVector vecXYZ = mPlanetTransform * DoubleVector(flMaxX, flMaxY, flMaxZ);

	CSPCharacter* pLocalCharacter = SPGame()->GetLocalPlayerCharacter();
	CScalableVector vecRender = GetGlobalOrigin() - pLocalCharacter->GetGlobalOrigin();
	c.Translate(vecRender.GetUnits(SPGame()->GetSPRenderer()->GetRenderingScale()));
	c.BeginRenderDebugLines();
		c.Vertex(vecxyZ);
		c.Vertex(vecxyz);
		c.Vertex(vecxYz);
		c.Vertex(vecXYz);
		c.Vertex(vecXyz);
		c.Vertex(vecxyz);
	c.EndRender();
	c.BeginRenderDebugLines();
		c.Vertex(vecxyZ);
		c.Vertex(vecxYZ);
		c.Vertex(vecxYz);
		c.Vertex(vecxYZ);
		c.Vertex(vecXYZ);
		c.Vertex(vecXyZ);
	c.EndRender();
	c.BeginRenderDebugLines();
		c.Vertex(vecXyz);
		c.Vertex(vecXyZ);
	c.EndRender();
	c.BeginRenderDebugLines();
		c.Vertex(vecXYz);
		c.Vertex(vecXYZ);
	c.EndRender();
}

void CPlanet::Debug_RenderCollision(const COctreeBranch<CChunkOrQuad, double>* pBranch) const
{
	if (!pBranch)
		return;

	CScalableMatrix mPlanetTransform = GetGlobalTransform();

	scale_t eScale = SPGame()->GetSPRenderer()->GetRenderingScale();
	CSPCharacter* pLocalCharacter = SPGame()->GetLocalPlayerCharacter();
	CSPRenderer* pRenderer = SPGame()->GetSPRenderer();
	CScalableVector vecDistanceToBranch = mPlanetTransform * CScalableVector(pBranch->m_oBounds.Center(), GetScale()) - pLocalCharacter->GetGlobalOrigin();
	float flGlobalRadius = (float)CScalableFloat(pBranch->m_oBounds.Size().Length(), GetScale()).GetUnits(eScale);
	if (!pRenderer->IsInFrustumAtScale(eScale, vecDistanceToBranch.GetUnits(eScale), flGlobalRadius))
		return;

	mPlanetTransform.SetTranslation(Vector());

	if (pBranch->m_pBranches[0])
	{
		for (size_t i = 0; i < 8; i++)
			Debug_RenderCollision(pBranch->m_pBranches[i]);
		return;
	}

	if (!pBranch->m_aObjects.size())
		return;

	for (size_t i = 0; i < pBranch->m_aObjects.size(); i++)
	{
		CQuadTreeBranch<CBranchData, double>* pQuad = pBranch->m_aObjects[i].m_oData.m_pQuad;
		if (!pQuad)
			continue;

		if (r_planetcollision.GetInt() >= 0 && pQuad->m_iDepth != r_planetcollision.GetInt())
			continue;

		CRenderingContext c(GameServer()->GetRenderer());

		DoubleVector vec1 = (mPlanetTransform * CScalableVector(pQuad->m_oData.vec1, GetScale())).GetUnits(eScale);
		DoubleVector vec2 = (mPlanetTransform * CScalableVector(pQuad->m_oData.vec2, GetScale())).GetUnits(eScale);
		//DoubleVector vec3 = (mPlanetTransform * CScalableVector(pQuad->m_oData.vec3, GetScale())).GetUnits(eScale);
		DoubleVector vec4 = (mPlanetTransform * CScalableVector(pQuad->m_oData.vec4, GetScale())).GetUnits(eScale);

		CScalableVector vecRender = GetGlobalOrigin() - pLocalCharacter->GetGlobalOrigin();
		c.Translate(vecRender.GetUnits(eScale));
		c.BeginRenderDebugLines();
			c.Vertex(vec1);
			c.Vertex(vec2);
			c.Vertex(vec4);
		c.EndRender();
	}
}

bool CChunkOrQuad::Raytrace(const DoubleVector& vecStart, const DoubleVector& vecEnd, CCollisionResult& tr)
{
	if (m_pQuad)
	{
		scale_t eScale = static_cast<CPlanetTerrain*>(m_pQuad->m_pTree)->GetPlanet()->GetScale();
		CScalableVector vec1 = CScalableVector(m_pQuad->m_oData.vec1, eScale);
		CScalableVector vec2 = CScalableVector(m_pQuad->m_oData.vec2, eScale);
		CScalableVector vec3 = CScalableVector(m_pQuad->m_oData.vec3, eScale);
		CScalableVector vec4 = CScalableVector(m_pQuad->m_oData.vec4, eScale);
		CScalableVector v1 = CScalableVector(vecStart, eScale);
		CScalableVector v2 = CScalableVector(vecEnd, eScale);

		if (LineSegmentIntersectsTriangle(vecStart, vecEnd, m_pQuad->m_oData.vec1, m_pQuad->m_oData.vec2, m_pQuad->m_oData.vec3, tr))
			return true;
		if (LineSegmentIntersectsTriangle(vecStart, vecEnd, m_pQuad->m_oData.vec2, m_pQuad->m_oData.vec4, m_pQuad->m_oData.vec3, tr))
			return true;

		return false;
	}
	else if (m_iChunk != ~0)
	{
		CTerrainChunk* pChunk = m_pPlanet->m_pTerrainChunkManager->GetChunk(m_iChunk);
		if (!pChunk)
			return false;

		return pChunk->Raytrace(vecStart, vecEnd, tr);
	}
	else
	{
		TAssert(m_pQuad || m_iChunk != ~0);
		return false;
	}
}
