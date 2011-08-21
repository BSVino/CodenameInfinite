#include "planet.h"

#include <geometry.h>

#include <renderer/renderingcontext.h>
#include <tinker/cvar.h>
#include <tinker/application.h>
#include <tinker/profiler.h>

#include "sp_game.h"
#include "sp_renderer.h"
#include "sp_camera.h"
#include "sp_character.h"

REGISTER_ENTITY(CPlanet);

NETVAR_TABLE_BEGIN(CPlanet);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CPlanet);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CScalableFloat, m_flRadius);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CScalableFloat, m_flAtmosphereThickness);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, float, m_flMinutesPerRevolution);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, int, m_iMinQuadRenderDepth);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, bool, m_bOneSurface);
	SAVEDATA_DEFINE(CSaveData::DATA_STRING, tstring, m_sPlanetName);
	SAVEDATA_OMIT(m_pTerrainFd);
	SAVEDATA_OMIT(m_pTerrainBk);
	SAVEDATA_OMIT(m_pTerrainRt);
	SAVEDATA_OMIT(m_pTerrainLf);
	SAVEDATA_OMIT(m_pTerrainUp);
	SAVEDATA_OMIT(m_pTerrainDn);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CPlanet);
INPUTS_TABLE_END();

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
	PrecacheTexture("textures/planet.png");
}

void CPlanet::Spawn()
{
	// 1500km, a bit smaller than the moon
	SetRadius(CScalableFloat(1.5f, SCALE_MEGAMETER));
	// 20 km thick
	SetAtmosphereThickness(CScalableFloat(20.0f, SCALE_KILOMETER));
}

CVar r_planet_onesurface("r_planet_onesurface", "off");

void CPlanet::Think()
{
	TPROF("CPlanet::Think");

	BaseClass::Think();

	SetLocalScalableAngles(GetLocalScalableAngles() + EAngle(0, 360, 0)*(GameServer()->GetFrameTime()/60/m_flMinutesPerRevolution));

	m_bOneSurface = r_planet_onesurface.GetBool();
}

static DoubleVector g_vecCharacterLocalOrigin;

void CPlanet::RenderUpdate()
{
	TPROF("CPlanet::RenderUpdate");

	Vector vecUp;
	Vector vecForward;
	GameServer()->GetRenderer()->GetCameraVectors(&vecForward, NULL, &vecUp);

	CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
	CScalableVector vecCharacterOrigin = pCharacter->GetGlobalScalableOrigin();

	// Transforming every quad to global coordinates in ShouldRenderBranch() is expensive.
	// Instead, transform the player to the planet's local once and do the math in local space.
	CScalableMatrix mPlanetGlobalToLocal = GetGlobalScalableTransform();
	mPlanetGlobalToLocal.InvertTR();
	g_vecCharacterLocalOrigin = DoubleVector((mPlanetGlobalToLocal * vecCharacterOrigin).GetUnits(GetScale()));

	Vector vecOrigin = (GetGlobalScalableOrigin() - vecCharacterOrigin).GetUnits(GetScale());
	Vector vecOutside = vecOrigin + vecUp * GetScalableRenderRadius().GetUnits(GetScale());

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

CScalableFloat CPlanet::GetScalableRenderRadius() const
{
	return m_flRadius;
}

CVar r_colorcodescales("r_colorcodescales", "off");

void CPlanet::PostRender(bool bTransparent) const
{
	BaseClass::PostRender(bTransparent);

	if (bTransparent)
		return;

	TPROF("CPlanet::PostRender");

	scale_t eScale = SPGame()->GetSPRenderer()->GetRenderingScale();

	CRenderingContext c(GameServer()->GetRenderer());

	c.SetBackCulling(false);	// Ideally this would be on but it's not a big deal.
	c.BindTexture("textures/planet.png");

	if (r_colorcodescales.GetBool())
	{
		if (eScale == SCALE_KILOMETER)
			c.SetColor(Color(0, 0, 255));
		else if (eScale == SCALE_MEGAMETER)
			c.SetColor(Color(0, 255, 0));
		else if (eScale == SCALE_GIGAMETER)
			c.SetColor(Color(255, 0, 0));
		else
			c.SetColor(Color(255, 255, 255));
	}
	else
		c.SetColor(Color(255, 255, 255));

	for (size_t i = 0; i < (size_t)(m_bOneSurface?1:6); i++)
		m_pTerrain[i]->Render(&c);
}

CScalableFloat CPlanet::GetCloseOrbit()
{
	// For Earth values this resolves to about 600km above the ground, or about twice the altitude of the ISS.
	return GetRadius()/10;
}

void CPlanetTerrain::Init()
{
	CBranchData oData;
	oData.bRender = true;
	oData.flHeight = 0;
	oData.flScreenSize = 1;
	oData.flLastScreenUpdate = -1;
	oData.iRenderVectorsLastFrame = ~0;
	oData.iShouldRenderLastFrame = ~0;
	CTerrainQuadTree::Init(this, oData);
}

CVar r_terrain_onequad("r_terrain_onequad", "off");

void CPlanetTerrain::Think()
{
	m_bOneQuad = r_terrain_onequad.GetBool();
	m_apRenderBranches.clear();
	m_iBuildsThisFrame = 0;
	ThinkBranch(m_pQuadTreeHead);
}

CVar r_terrainbuildsperframe("r_terrainbuildsperframe", "10");

void CPlanetTerrain::ThinkBranch(CTerrainQuadTreeBranch* pBranch)
{
	if (pBranch->m_oData.bRender)
	{
		// If I can render then maybe my kids can too?
		if (!pBranch->m_pBranches[0] && m_iBuildsThisFrame < r_terrainbuildsperframe.GetInt())
		{
			pBranch->BuildBranch(false);

			if (pBranch->m_pBranches[0])
				m_iBuildsThisFrame++;
		}

		// See if we can push the render surface down to the next level.
		bool bCanRenderKids = false;
		if (pBranch->m_pBranches[0])
		{
			for (size_t i = 0; i < (size_t)(m_bOneQuad?1:4); i++)
			{
				bCanRenderKids |= ShouldRenderBranch(pBranch->m_pBranches[i]);
				if (bCanRenderKids)
					break;
			}
		}
		else
			bCanRenderKids = false;

		if (bCanRenderKids)
		{
			pBranch->m_oData.bRender = false;
			for (size_t i = 0; i < (size_t)(m_bOneQuad?1:4); i++)
				pBranch->m_pBranches[i]->m_oData.bRender = true;

			for (size_t i = 0; i < (size_t)(m_bOneQuad?1:4); i++)
				ThinkBranch(pBranch->m_pBranches[i]);

			return;
		}

		ProcessBranchRendering(pBranch);
		return;
	}

	TAssert(pBranch->m_pBranches[0]);

	// See if we can pull the render surface up from our kids.
	bool bAreKidsRenderingAndShouldnt = true;
	if (pBranch->m_pBranches[0])
	{
		for (size_t i = 0; i < (size_t)(m_bOneQuad?1:4); i++)
		{
			bAreKidsRenderingAndShouldnt &= (pBranch->m_pBranches[i]->m_oData.bRender && !ShouldRenderBranch(pBranch->m_pBranches[i]));
			if (!bAreKidsRenderingAndShouldnt)
				break;
		}
	}

	if (bAreKidsRenderingAndShouldnt)
	{
		pBranch->m_oData.bRender = true;
		for (size_t i = 0; i < (size_t)(m_bOneQuad?1:4); i++)
			pBranch->m_pBranches[i]->m_oData.bRender = false;

		ProcessBranchRendering(pBranch);
		return;
	}
	else
	{
		// My kids should be rendering.
		for (size_t i = 0; i < (size_t)(m_bOneQuad?1:4); i++)
			ThinkBranch(pBranch->m_pBranches[i]);
	}
}

void CPlanetTerrain::ProcessBranchRendering(CTerrainQuadTreeBranch* pBranch)
{
	TAssert(pBranch->m_oData.bRender);

	CalcRenderVectors(pBranch);
	UpdateScreenSize(pBranch);

	CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
	CSPRenderer* pRenderer = SPGame()->GetSPRenderer();

	CScalableVector vecDistanceToQuad = pBranch->m_oData.vecGlobalQuadCenter - pCharacter->GetGlobalScalableOrigin();

	int iPushes = 0;
	for (size_t i = 0; i < SCALESTACK_SIZE; i++)
	{
		scale_t eScale = (scale_t)(i+1);

		if (pRenderer->IsInFrustumAtScale(eScale, vecDistanceToQuad.GetUnits(eScale), pBranch->m_oData.flGlobalRadius.GetUnits(eScale)))
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

void CPlanetTerrain::RenderBranch(const CTerrainQuadTreeBranch* pBranch, class CRenderingContext* c) const
{
	TAssert(pBranch->m_oData.iRenderVectorsLastFrame == GameServer()->GetFrame());

	scale_t ePlanet = m_pPlanet->GetScale();
	scale_t eRender = SPGame()->GetSPRenderer()->GetRenderingScale();
	CScalableMatrix mPlanetTransform = m_pPlanet->GetGlobalScalableTransform();
	CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();

	CScalableVector svec1;
	CScalableVector svec2;
	CScalableVector svec3;
	CScalableVector svec4;

	if (pCharacter->GetScalableMoveParent() == m_pPlanet)
	{
		CScalableVector vecCharacterOrigin = pCharacter->GetLocalScalableOrigin();

		svec1 = mPlanetTransform.TransformNoTranslate(CScalableVector(pBranch->m_oData.vec1, ePlanet) - vecCharacterOrigin);
		svec2 = mPlanetTransform.TransformNoTranslate(CScalableVector(pBranch->m_oData.vec2, ePlanet) - vecCharacterOrigin);
		svec3 = mPlanetTransform.TransformNoTranslate(CScalableVector(pBranch->m_oData.vec3, ePlanet) - vecCharacterOrigin);
		svec4 = mPlanetTransform.TransformNoTranslate(CScalableVector(pBranch->m_oData.vec4, ePlanet) - vecCharacterOrigin);
	}
	else
	{
		CScalableVector vecCharacterOrigin = pCharacter->GetGlobalScalableOrigin();

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
	c->TexCoord(pBranch->m_vecMin);
	c->Normal(pBranch->m_oData.vec1n);
	c->Vertex(vec1);
	c->TexCoord(DoubleVector2D(pBranch->m_vecMax.x, pBranch->m_vecMin.y));
	c->Normal(pBranch->m_oData.vec2n);
	c->Vertex(vec2);
	c->TexCoord(pBranch->m_vecMax);
	c->Normal(pBranch->m_oData.vec3n);
	c->Vertex(vec3);
	c->TexCoord(DoubleVector2D(pBranch->m_vecMin.x, pBranch->m_vecMax.y));
	c->Normal(pBranch->m_oData.vec4n);
	c->Vertex(vec4);
	c->EndRender();

	if (r_showquaddebugoutlines.GetBool())
	{
		CRenderingContext c(GameServer()->GetRenderer());
		c.BindTexture(0);
		c.SetColor(Color(255, 255, 255));
		c.BeginRenderDebugLines();
		c.Vertex(vec1);
		c.Vertex(vec2);
		c.Vertex(vec3);
		c.Vertex(vec4);
		c.EndRender();
	}

	if (r_showquadnormals.GetBool())
	{
		CRenderingContext c(GameServer()->GetRenderer());
		c.BindTexture(0);
		c.SetColor(Color(255, 255, 255));
		c.BeginRenderDebugLines();
		c.Vertex(vec1);
		c.Vertex(vec1 + pBranch->m_oData.vec1n);
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

		CScalableMatrix mPlanet = m_pPlanet->GetGlobalScalableTransform();
		CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
		CScalableVector vecCharacterOrigin = pCharacter->GetGlobalScalableOrigin();

		CScalableVector vecQuadCenter = pBranch->m_oData.vecGlobalQuadCenter - vecCharacterOrigin;
		CScalableVector vecQuadMax = mPlanet * CScalableVector(QuadTreeToWorld(this, pBranch->m_vecMax), m_pPlanet->GetScale()) - vecCharacterOrigin;
		CScalableFloat flRadius = (vecQuadCenter-vecQuadMax).Length();

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

CVar r_terrainbackfacecull("r_terrainbackfacecull", "on");
CVar r_terrainperspectivescale("r_terrainperspectivescale", "on");
CVar r_terrainfrustumcull("r_terrainfrustumcull", "on");

bool CPlanetTerrain::ShouldRenderBranch(CTerrainQuadTreeBranch* pBranch)
{
	if (pBranch->m_oData.iShouldRenderLastFrame == GameServer()->GetFrame())
		return pBranch->m_oData.bShouldRender;

	pBranch->m_oData.iShouldRenderLastFrame = GameServer()->GetFrame();
	pBranch->m_oData.bShouldRender = false;

	CalcRenderVectors(pBranch);

	if (pBranch->m_oData.flRadiusMeters < r_terrainresolution.GetFloat())
		return false;

	DoubleVector vecNormal = DoubleVector(pBranch->m_oData.vec1n + pBranch->m_oData.vec3n)/2;

	float flDot = (float)(pBranch->GetCenter()-g_vecCharacterLocalOrigin).Normalized().Dot(vecNormal);

	if (r_terrainbackfacecull.GetBool() && flDot >= 0.4f)
		return false;

	float flScale = RemapValClamped(flDot, -1, 1, 0.1f, 1);

	if (!r_terrainperspectivescale.GetBool())
		flScale = 1;

	UpdateScreenSize(pBranch);

	if (pBranch->m_iDepth > m_pPlanet->GetMinQuadRenderDepth() && pBranch->m_oData.flScreenSize*flScale < r_minterrainsize.GetFloat())
		return false;

	if (r_terrainfrustumcull.GetBool())
	{
		CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
		CScalableMatrix mPlanet = m_pPlanet->GetGlobalScalableTransform();
		CScalableVector vecPlanetCenter = pBranch->m_oData.vecGlobalQuadCenter - pCharacter->GetGlobalScalableOrigin();

		Vector vecPlanetCenterUnscaled = vecPlanetCenter.GetUnits(m_pPlanet->GetScale());
		float flRadiusUnscaled = CScalableFloat(pBranch->m_oData.flRadiusMeters, SCALE_METER).GetUnits(m_pPlanet->GetScale());

		if (!SPGame()->GetSPRenderer()->IsInFrustumAtScaleSidesOnly(m_pPlanet->GetScale(), vecPlanetCenterUnscaled, flRadiusUnscaled))
			return false;
	}

	pBranch->m_oData.bShouldRender = true;
	return true;
}

void CPlanetTerrain::CalcRenderVectors(CTerrainQuadTreeBranch* pBranch)
{
	if (pBranch->m_oData.flRadiusMeters == 0)
	{
		pBranch->m_oData.vec1 = m_pDataSource->QuadTreeToWorld(this, pBranch->m_vecMin);
		pBranch->m_oData.vec2 = m_pDataSource->QuadTreeToWorld(this, DoubleVector2D(pBranch->m_vecMax.x, pBranch->m_vecMin.y));
		pBranch->m_oData.vec3 = m_pDataSource->QuadTreeToWorld(this, pBranch->m_vecMax);
		pBranch->m_oData.vec4 = m_pDataSource->QuadTreeToWorld(this, DoubleVector2D(pBranch->m_vecMin.x, pBranch->m_vecMax.y));

		pBranch->m_oData.vec1n = Vector(pBranch->m_oData.vec1).Normalized();
		pBranch->m_oData.vec2n = Vector(pBranch->m_oData.vec2).Normalized();
		pBranch->m_oData.vec3n = Vector(pBranch->m_oData.vec3).Normalized();
		pBranch->m_oData.vec4n = Vector(pBranch->m_oData.vec4).Normalized();

		CScalableVector vecQuadCenter(pBranch->GetCenter(), m_pPlanet->GetScale());
		CScalableVector vecQuadMax(pBranch->m_oData.vec3, m_pPlanet->GetScale());

		CScalableFloat flRadius = (vecQuadCenter - vecQuadMax).Length();
		pBranch->m_oData.flRadiusMeters = flRadius.GetUnits(SCALE_METER);
	}

	if (pBranch->m_oData.iRenderVectorsLastFrame == GameServer()->GetFrame())
		return;

	pBranch->m_oData.iRenderVectorsLastFrame = GameServer()->GetFrame();

	pBranch->m_oData.vecGlobalQuadCenter = m_pPlanet->GetGlobalScalableTransform() * CScalableVector(pBranch->GetCenter(), m_pPlanet->GetScale());
}

DoubleVector2D CPlanetTerrain::WorldToQuadTree(const CTerrainQuadTree* pTree, const DoubleVector& v) const
{
	TAssert(pTree == (CTerrainQuadTree*)this);

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

	if (vecDirection[0] != 0)
		vecCubePoint = DoubleVector(vecDirection[0], x, y);
	else if (vecDirection[1] != 0)
		vecCubePoint = DoubleVector(x, vecDirection[1], y);
	else
		vecCubePoint = DoubleVector(x, y, vecDirection[2]);

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

bool CPlanetTerrain::ShouldBuildBranch(CTerrainQuadTreeBranch* pBranch, bool& bDelete)
{
	bDelete = false;

	return ShouldRenderBranch(pBranch);
}
