#include "planet.h"

#include <geometry.h>

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

	m_pTerrainChunkManager = new CTerrainChunkManager(this);
}

CPlanet::~CPlanet()
{
	delete m_pTerrainChunkManager;

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

void CPlanet::PostRender(bool bTransparent) const
{
	BaseClass::PostRender(bTransparent);

	if (bTransparent)
		return;

	TPROF("CPlanet::PostRender");

	scale_t eScale = SPGame()->GetSPRenderer()->GetRenderingScale();

	if (debug_showplanetcollision.GetBool())
	{
		CSPCharacter* pLocalCharacter = SPGame()->GetLocalPlayerCharacter();
		Vector vecForward = GameServer()->GetRenderer()->GetCameraVector();
		CScalableVector vecUp = pLocalCharacter->GetUpVector();
		CTraceResult tr;
		
		if (pLocalCharacter->GetMoveParent() == this)
		{
			CScalableMatrix mGlobalToLocal = GetGlobalToLocalTransform();
			vecUp = mGlobalToLocal.TransformNoTranslate(vecUp);
			vecForward = mGlobalToLocal.TransformNoTranslate(vecForward);
			CScalableVector vecCharacter = pLocalCharacter->GetLocalOrigin() + vecUp * pLocalCharacter->EyeHeight();
			// I would normally never use const_cast if this weren't a block of debug code.
			const_cast<CPlanet*>(this)->CollideLocal(vecCharacter, vecCharacter + vecForward * CScalableFloat(100.0f, SCALE_KILOMETER), tr);
			tr.vecHit = GetGlobalTransform() * tr.vecHit;
			tr.vecNormal = GetGlobalTransform().TransformNoTranslate(tr.vecNormal);
		}
		else
		{
			CScalableVector vecCharacter = pLocalCharacter->GetGlobalOrigin() + vecUp * pLocalCharacter->EyeHeight();
			const_cast<CPlanet*>(this)->Collide(vecCharacter, vecCharacter + vecForward * CScalableFloat(100.0f, SCALE_GIGAMETER), tr);
		}

		if (tr.bHit)
		{
			CRenderingContext c(GameServer()->GetRenderer());

			CScalableVector vecRender = tr.vecHit - pLocalCharacter->GetGlobalOrigin();
			c.Translate((tr.vecHit - pLocalCharacter->GetGlobalOrigin()).GetUnits(SPGame()->GetSPRenderer()->GetRenderingScale()));
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

	CStar* pStar = SPGame()->GetSPRenderer()->GetClosestStar();
	CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
	CScalableMatrix mPlanetToLocal = GetGlobalTransform();
	mPlanetToLocal.InvertTR();

	Vector vecStarLightPosition = (mPlanetToLocal.TransformNoTranslate(pStar->GetScalableRenderOrigin())).GetUnits(eScale);

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

bool CPlanet::CollideLocal(const TVector& v1, const TVector& v2, CTraceResult& tr)
{
	return CollideLocalAccurate(false, v1, v2, tr);
}

bool CPlanet::CollideLocalAccurate(bool bAccurate, const TVector& v1, const TVector& v2, CTraceResult& tr)
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

	for (size_t i = 0; i < 6; i++)
		m_pTerrain[i]->CollideLocal(bAccurate, v1, v2, tr);

	if (tr.bHit)
		tr.pHit = this;

	return tr.bHit;
}

bool CPlanet::Collide(const TVector& v1, const TVector& v2, CTraceResult& tr)
{
	return CollideAccurate(false, v1, v2, tr);
}

bool CPlanet::CollideAccurate(bool bAccurate, const TVector& v1, const TVector& v2, CTraceResult& tr)
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

	for (size_t i = 0; i < 6; i++)
		m_pTerrain[i]->Collide(bAccurate, v1, v2, tr);

	if (tr.bHit)
		tr.pHit = this;

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
