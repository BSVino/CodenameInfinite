#include "planet.h"

#include <geometry.h>
#include <parallelize.h>

#include <renderer/renderingcontext.h>
#include <tinker/cvar.h>
#include <tinker/application.h>
#include <tinker/profiler.h>
#include <renderer/shaders.h>
#include <tengine/renderer/game_renderer.h>

#include "entities/sp_game.h"
#include "sp_renderer.h"
#include "entities/sp_camera.h"
#include "entities/sp_playercharacter.h"
#include "entities/star.h"
#include "planet/planet_terrain.h"
#include "planet/terrain_lumps.h"
#include "planet/terrain_chunks.h"

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

CParallelizer* CPlanet::s_pShell2Generator = nullptr;

Vector g_vecTerrainDirections[] =
{
	Vector(1, 0, 0),
	Vector(-1, 0, 0),
	Vector(0, 0, 1),
	Vector(0, 0, -1),
	Vector(0, 1, 0),
	Vector(0, -1, 0),
};

void GenerateShell2Callback(void* pJob)
{
	CShell2GenerationJob* pShellJob = reinterpret_cast<CShell2GenerationJob*>(pJob);

	pShellJob->pTerrain->CreateShell2VBO();
}

CPlanet::CPlanet()
{
	if (!s_pShell2Generator)
	{
		s_pShell2Generator = new CParallelizer(GenerateShell2Callback, 1);
		s_pShell2Generator->Start();
	}

	m_iRandomSeed = 0;

	for (size_t i = 0; i < 6; i++)
		m_apTerrain[i] = new CPlanetTerrain(this, g_vecTerrainDirections[i]);

	m_pLumpManager = new CTerrainLumpManager(this);
	m_pChunkManager = new CTerrainChunkManager(this);

	m_iMinQuadRenderDepth = 3;
}

CPlanet::~CPlanet()
{
	delete m_pLumpManager;
	delete m_pChunkManager;

	for (size_t i = 0; i < 6; i++)
		delete m_apTerrain[i];
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

CVar r_minquadrenderdepth("r_minquadrenderdepth", "-1");

void CPlanet::RenderUpdate()
{
	TPROF("CPlanet::RenderUpdate");

	Vector vecUp;
	Vector vecForward;
	GameServer()->GetRenderer()->GetCameraVectors(&vecForward, NULL, &vecUp);

	CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();

	m_vecCharacterLocalOrigin = (GetGlobalToLocalTransform() * pCharacter->GetGlobalOrigin()).GetUnits(GetScale());

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

	m_pLumpManager->Think();
	m_pChunkManager->Think();

	for (size_t i = 0; i < (size_t)(m_bOneSurface?1:6); i++)
		m_apTerrain[i]->Think();
}

bool CPlanet::ShouldRenderAtScale(scale_t eScale) const
{
	if (eScale >= SCALE_MEGAMETER)
//	for (size_t i = 0; i < (size_t)(m_bOneSurface?1:6); i++)
//		if (m_apTerrain[i]->m_apRenderBranches.find(eScale) != m_apTerrain[i]->m_apRenderBranches.end())
			return true;

	return false;
}

const CScalableFloat CPlanet::GetRenderRadius() const
{
	return m_flRadius + GetAtmosphereThickness();
}

CVar r_colorcodescales("r_colorcodescales", "off");
CVar debug_showplanetcollision("debug_showplanetcollision", "off");
CVar r_planets("r_planets", "on");
CVar r_planet_shells("r_planet_shells", "on");
CVar r_planet_lumps("r_planet_lumps", "on");
CVar r_planet_chunks("r_planet_chunks", "on");

void CPlanet::PostRender() const
{
	BaseClass::PostRender();

	if (GameServer()->GetRenderer()->IsRenderingTransparent())
		return;

	TPROF("CPlanet::PostRender");

	if (!r_planets.GetBool())
		return;

	scale_t eScale = SPGame()->GetSPRenderer()->GetRenderingScale();
	if (eScale == SCALE_RENDER)
		return;

	CStar* pStar = SPGame()->GetSPRenderer()->GetClosestStar();
	CPlayerCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
	CScalableMatrix mPlanetToLocal = GetGlobalTransform();
	mPlanetToLocal.InvertRT();

	Vector vecStarLightPosition;
	if (pCharacter->GetNearestPlanet() == this)
		vecStarLightPosition = pStar->GameData().GetScalableRenderOrigin();
	else
		vecStarLightPosition = (mPlanetToLocal.TransformVector(pStar->GameData().GetScalableRenderOrigin())).GetUnits(eScale);

	float flScale;
	if (eScale == SCALE_RENDER)
		flScale = (float)CScalableFloat::ConvertUnits(1, GetScale(), SCALE_METER);
	else
		flScale = (float)CScalableFloat::ConvertUnits(1, GetScale(), eScale);

	CRenderingContext c(GameServer()->GetRenderer(), true);

	c.ResetTransformations();

	c.Transform(GameData().GetScalableRenderTransform().GetUnits(eScale));
	c.Scale(flScale, flScale, flScale);

	c.UseMaterial("textures/earth.mat");

	c.SetUniform("bDetail", false);
	c.SetUniform("vecStarLightPosition", vecStarLightPosition);
	c.SetUniform("eScale", eScale);
	c.SetUniform("flScale", flScale);

	CScalableFloat flDistance = (GetGlobalOrigin() - pCharacter->GetGlobalOrigin()).Length() - GetRadius();
	float flAtmosphere = (float)RemapValClamped(flDistance, CScalableFloat(1.0f, SCALE_KILOMETER), GetAtmosphereThickness(), 1.0, 0.0);
	c.SetUniform("flAtmosphere", flAtmosphere);

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

	bool bShowShells = true;

	if (pCharacter->GetNearestPlanet() == this)
	{
		double flElevation = pCharacter->GetApproximateElevation();
		double flNoShellElevation = flElevation + 0.0001;
		double flPlayerDistanceSqr = m_vecCharacterLocalOrigin.LengthSqr();
		if (flPlayerDistanceSqr < flNoShellElevation*flNoShellElevation)
			bShowShells = false;
	}

	if (bShowShells && r_planet_shells.GetBool())
	{
		for (size_t i = 0; i < (size_t)(m_bOneSurface?1:6); i++)
			m_apTerrain[i]->Render(&c);
	}

	if (r_planet_lumps.GetBool())
		m_pLumpManager->Render();

	if (r_planet_chunks.GetBool())
		m_pChunkManager->Render();
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

void CPlanet::GetApprox2DPosition(const DoubleVector& vec3DLocal, size_t& iTerrain, DoubleVector2D& vec2DCoord)
{
	// Find the spot on the planet's surface that this point is closest to.
	double flHighestDot = -1;

	DoubleVector vecPointDirection = vec3DLocal.Normalized();

	for (size_t i = 0; i < 6; i++)
	{
		DoubleVector vecDirection = GetTerrain(i)->GetDirection();

		double flDot = vecPointDirection.Dot(vecDirection);

		if (flDot <= flHighestDot)
			continue;

		flHighestDot = flDot;

		float flXSign = 1, flYSign = 1;
		int iBig;
		int iX;
		int iY;
		if (vecDirection[0] != 0)
		{
			iBig = 0;
			iX = 2;
			iY = 1;

			if (vecDirection[0] > 0)
				flXSign = -1;
			else
			{
				flXSign = -1;
				flYSign = -1;
			}
		}
		else if (vecDirection[1] != 0)
		{
			iBig = 1;
			iX = 2;
			iY = 0;

			flXSign = -1;
			if (vecDirection[1] > 0)
				flYSign = -1;
		}
		else
		{
			iBig = 2;
			iX = 0;
			iY = 1;

			if (vecDirection[2] < 0)
				flXSign = -1;
		}

		// Shoot a ray from (0,0,0) through (a,b,c) until it hits a face of the bounding cube.
		// The faces of the bounding cube are specified by GetDirection() above.
		// If the ray hits the +z face at (x,y,z) then (x,y,1) is a scalar multiple of (a,b,c).
		// (x,y,1) = r*(a,b,c), so 1 = r*c and r = 1/c
		// Then we can figure x and y with x = a*r and y = b*r

		double r = 1/vecPointDirection[iBig];

		double x = RemapVal(flXSign*vecPointDirection[iX]*r, -1, 1, 0, 1);
		double y = RemapVal(flYSign*vecPointDirection[iY]*r, -1, 1, 0, 1);

		iTerrain = i;
		vec2DCoord = DoubleVector2D(x, y);
	}

	TAssert(vec2DCoord.x >= 0);
	TAssert(vec2DCoord.y >= 0);
	TAssert(vec2DCoord.x <= 1);
	TAssert(vec2DCoord.y <= 1);
}

void CPlanet::SetRadius(const CScalableFloat& flRadius)
{
	m_flRadius = flRadius;

	DoubleVector vec1 = m_apTerrain[0]->CoordToWorld(DoubleVector2D(0, 0));
	DoubleVector vec2 = m_apTerrain[0]->CoordToWorld(DoubleVector2D(1, 0));
	DoubleVector vec3 = m_apTerrain[0]->CoordToWorld(DoubleVector2D(1, 1));
	DoubleVector vec4 = m_apTerrain[0]->CoordToWorld(DoubleVector2D(0, 1));

	DoubleVector vecCenter = (vec1 + vec2 + vec3 + vec4)/4;

	CScalableVector vecQuadCenter(vecCenter, GetScale());

	CScalableVector vecQuadMax1(vec1, GetScale());
	CScalableFloat flRadius1 = (vecQuadCenter - vecQuadMax1).Length();
	CScalableVector vecQuadMax2(vec2, GetScale());
	CScalableFloat flRadius2 = (vecQuadCenter - vecQuadMax2).Length();
	CScalableVector vecQuadMax3(vec3, GetScale());
	CScalableFloat flRadius3 = (vecQuadCenter - vecQuadMax3).Length();
	CScalableVector vecQuadMax4(vec4, GetScale());
	CScalableFloat flRadius4 = (vecQuadCenter - vecQuadMax4).Length();

	CScalableFloat flTerrainRadius = flRadius1;
	if (flRadius2 > flRadius)
		flTerrainRadius = flRadius2;
	if (flRadius3 > flRadius)
		flTerrainRadius = flRadius3;
	if (flRadius4 > flRadius)
		flTerrainRadius = flRadius4;

	float flRadiusMeters = (float)flTerrainRadius.GetUnits(SCALE_METER);

	m_iMeterDepth = (int)(log(flRadiusMeters)/log(2.0f));
	m_iLumpDepth = 7;
}

CScalableFloat CPlanet::GetCloseOrbit()
{
	// For Earth values this resolves to about 600km above the ground, or about twice the altitude of the ISS.
	return GetRadius()/10.0f;
}


void Planet_RebuildTerrain(class CCommand* pCommand, tvector<tstring>& asTokens, const tstring& sCommand)
{
	CPlayerCharacter* pLocalPlayer = SPGame()->GetLocalPlayerCharacter();
	CPlanet* pNearestPlanet = pLocalPlayer->GetNearestPlanet(FINDPLANET_ANY);

	pNearestPlanet->Debug_RebuildTerrain();
}

CCommand planet_terrainrebuild("planet_terrainrebuild", Planet_RebuildTerrain);

void CPlanet::Debug_RebuildTerrain()
{
	for (size_t i = 0; i < 6; i++)
		delete m_apTerrain[i];

	for (size_t i = 0; i < 6; i++)
		m_apTerrain[i] = new CPlanetTerrain(this, g_vecTerrainDirections[i]);
}
