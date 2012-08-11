#include "planet.h"

#include <geometry.h>

#include <renderer/renderingcontext.h>
#include <tinker/cvar.h>
#include <tinker/application.h>
#include <tinker/profiler.h>
#include <renderer/shaders.h>
#include <tengine/renderer/game_renderer.h>

#include "sp_game.h"
#include "sp_renderer.h"
#include "sp_camera.h"
#include "sp_character.h"
#include "star.h"
#include "planet_terrain.h"

REGISTER_ENTITY(CPlanet);

NETVAR_TABLE_BEGIN(CPlanet);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CPlanet);
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
	for (size_t i = 0; i < 6; i++)
	{
		m_pTerrain[i] = new CPlanetTerrain(this, g_vecTerrainDirections[i]);
		m_pTerrain[i]->Init();
	}
}

CPlanet::~CPlanet()
{
	for (size_t i = 0; i < 6; i++)
		delete m_pTerrain[i];
}

void CPlanet::Precache()
{
	PrecacheMaterial("textures/earth.mat");
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

	SetLocalAngles(GetLocalAngles() + EAngle(0, 360, 0)*((float)GameServer()->GetFrameTime()/60/m_flMinutesPerRevolution*planet_rotscale.GetFloat()));

	m_bOneSurface = r_planet_onesurface.GetBool();
}

void CPlanet::RenderUpdate()
{
	TPROF("CPlanet::RenderUpdate");

	Vector vecUp;
	Vector vecForward;
	GameServer()->GetRenderer()->GetCameraVectors(&vecForward, NULL, &vecUp);

	CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
	if (pCharacter->GetMoveParent() == this)
	{
		s_vecCharacterLocalOrigin = DoubleVector(pCharacter->GetLocalOrigin().GetUnits(GetScale()));
	}
	else
	{
		CScalableVector vecCharacterOrigin = pCharacter->GetGlobalOrigin();

		// Transforming every quad to global coordinates in ShouldRenderBranch() is expensive.
		// Instead, transform the player to the planet's local once and do the math in local space.
		CScalableMatrix mPlanetGlobalToLocal = GetGlobalTransform();
		mPlanetGlobalToLocal.InvertRT();
		s_vecCharacterLocalOrigin = DoubleVector((mPlanetGlobalToLocal * vecCharacterOrigin).GetUnits(GetScale()));
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

const CScalableFloat CPlanet::GetRenderRadius() const
{
	return m_flRadius;
}

CVar r_colorcodescales("r_colorcodescales", "off");
CVar debug_showplanetcollision("debug_showplanetcollision", "off");

void CPlanet::PostRender() const
{
	BaseClass::PostRender();

	if (GameServer()->GetRenderer()->IsRenderingTransparent())
		return;

	TPROF("CPlanet::PostRender");

	scale_t eScale = SPGame()->GetSPRenderer()->GetRenderingScale();

	if (debug_showplanetcollision.GetBool())
	{
		CSPCharacter* pLocalCharacter = SPGame()->GetLocalPlayerCharacter();
		CScalableVector vecPoint, vecNormal;
		Vector vecForward = GameServer()->GetRenderer()->GetCameraVector();
		CScalableVector vecUp = pLocalCharacter->GetUpVector();
		bool bIntersect;
		
		if (pLocalCharacter->GetMoveParent() == this)
		{
			CScalableMatrix mGlobalToLocal = GetGlobalToLocalTransform();
			vecUp = mGlobalToLocal.TransformVector(vecUp);
			vecForward = mGlobalToLocal.TransformVector(vecForward);
			CScalableVector vecCharacter = pLocalCharacter->GetLocalOrigin() + vecUp * pLocalCharacter->EyeHeight();
			bIntersect = LineSegmentIntersectsSphere(vecCharacter, vecCharacter + vecForward * CScalableFloat(100.0f, SCALE_KILOMETER), CScalableVector(), GetRadius(), vecPoint, vecNormal);
			vecPoint = GetGlobalTransform() * vecPoint;
			vecNormal = GetGlobalTransform() * vecNormal;
		}
		else
		{
			CScalableVector vecCharacter = pLocalCharacter->GetGlobalOrigin() + vecUp * pLocalCharacter->EyeHeight();
			bIntersect = LineSegmentIntersectsSphere(vecCharacter, vecCharacter + vecForward * CScalableFloat(100.0f, SCALE_KILOMETER), GetGlobalOrigin(), GetRadius(), vecPoint, vecNormal);
		}

		if (bIntersect)
		{
			CRenderingContext c(GameServer()->GetRenderer());

			CScalableVector vecRender = vecPoint - pLocalCharacter->GetGlobalOrigin();
			c.Translate((vecPoint - pLocalCharacter->GetGlobalOrigin()).GetUnits(SPGame()->GetSPRenderer()->GetRenderingScale()));
			c.SetColor(Color(255, 255, 255));
			c.BeginRenderDebugLines();
			c.Vertex(vecNormal*-10000);
			c.Vertex(vecNormal*10000);
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
			c.RenderSphere();
		}
	}

	CStar* pStar = SPGame()->GetSPRenderer()->GetClosestStar();
	CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
	CScalableMatrix mPlanetToLocal = GetGlobalTransform();
	mPlanetToLocal.InvertRT();

	Vector vecStarLightPosition = (mPlanetToLocal.TransformVector(pStar->GetScalableRenderOrigin())).GetUnits(eScale);

	CRenderingContext c(GameServer()->GetRenderer());

	c.UseMaterial("textures/earth.mat");

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
}

CScalableFloat CPlanet::GetCloseOrbit()
{
	// For Earth values this resolves to about 600km above the ground, or about twice the altitude of the ISS.
	return GetRadius()/10.0f;
}
