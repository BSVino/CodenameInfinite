#include "planet.h"

#include <geometry.h>

#include <tengine/renderer/renderer.h>
#include <tinker/cvar.h>
#include <tinker/application.h>

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
	SetAtmosphereThickness(CScalableFloat(20, SCALE_KILOMETER));
}

CVar r_planet_onesurface("r_planet_onesurface", "off");

void CPlanet::Think()
{
	BaseClass::Think();

	SetLocalScalableAngles(GetLocalScalableAngles() + EAngle(0, 360, 0)*(GameServer()->GetFrameTime()/60/m_flMinutesPerRevolution));

	m_bOneSurface = r_planet_onesurface.GetBool();

	for (size_t i = 0; i < (size_t)(m_bOneSurface?1:6); i++)
		m_pTerrain[i]->ResetRenderFlags();
}

void CPlanet::RenderUpdate()
{
	Vector vecUp;
	Vector vecForward;
	GameServer()->GetRenderer()->GetCameraVectors(&vecForward, NULL, &vecUp);

	scale_t eScale = SPGame()->GetSPRenderer()->GetRenderingScale();

	CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
	CScalableVector vecCharacterOrigin = pCharacter->GetGlobalScalableOrigin();

	Vector vecOrigin = (GetGlobalScalableOrigin() - vecCharacterOrigin).GetUnits(eScale);
	Vector vecOutside = vecOrigin + vecUp * GetScalableRenderRadius().GetUnits(eScale);

	Vector vecScreen = GameServer()->GetRenderer()->ScreenPosition(vecOrigin);
	Vector vecTop = GameServer()->GetRenderer()->ScreenPosition(vecOutside);

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
	oData.bRendered = false;
	oData.flHeight = 0;
	oData.flScreenSize = 1;
	oData.flLastScreenUpdate = -1;
	oData.bRenderVectorsDirty = true;
	CQuadTree<CBranchData>::Init(this, oData);
}

CVar r_terrain_onequad("r_terrain_onequad", "off");

void CPlanetTerrain::ResetRenderFlags(CQuadTreeBranch<CBranchData>* pBranch)
{
	if (pBranch)
	{
		if (pBranch->m_pBranches[0])
		{
			for (size_t i = 0; i < (size_t)(m_bOneQuad?1:4); i++)
				ResetRenderFlags(pBranch->m_pBranches[i]);
		}

		pBranch->m_oData.bRendered = false;
	}
	else
	{
		m_bOneQuad = r_terrain_onequad.GetBool();
		ResetRenderFlags(m_pQuadTreeHead);
	}
}

void CPlanetTerrain::Think()
{
	m_apRenderBranches.clear();
	m_iBuildsThisFrame = 0;
	ThinkBranch(m_pQuadTreeHead);
}

CVar r_terrainbuildsperframe("r_terrainbuildsperframe", "10");

void CPlanetTerrain::ThinkBranch(CQuadTreeBranch<CBranchData>* pBranch)
{
	if (pBranch->m_oData.bRendered)
		return;

	if (pBranch->m_oData.bRender)
	{
		// If I can render then maybe my kids can too?
		if (!pBranch->m_pBranches[0] && m_iBuildsThisFrame++ < r_terrainbuildsperframe.GetInt())
			pBranch->BuildBranch(false);

		// See if we can push the render surface down to the next level.
		bool bCanRenderKids = true;
		if (pBranch->m_pBranches[0])
		{
			for (size_t i = 0; i < (size_t)(m_bOneQuad?1:4); i++)
			{
				bCanRenderKids &= ShouldRenderBranch(pBranch->m_pBranches[i]);
				if (!bCanRenderKids)
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

			bool bAllKidsRendered = true;
			for (size_t i = 0; i < (size_t)(m_bOneQuad?1:4); i++)
			{
				ThinkBranch(pBranch->m_pBranches[i]);
				bAllKidsRendered &= pBranch->m_pBranches[i]->m_oData.bRendered;
			}

			if (bAllKidsRendered)
				pBranch->m_oData.bRendered = true;

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
		bool bAllKidsRendered = true;

		// My kids should be rendering.
		for (size_t i = 0; i < (size_t)(m_bOneQuad?1:4); i++)
		{
			ThinkBranch(pBranch->m_pBranches[i]);
			bAllKidsRendered &= pBranch->m_pBranches[i]->m_oData.bRendered;
		}

		if (bAllKidsRendered)
			pBranch->m_oData.bRendered = true;
	}
}

void CPlanetTerrain::ProcessBranchRendering(CQuadTreeBranch<CBranchData>* pBranch)
{
	TAssert(pBranch->m_oData.bRender);
	TAssert(!pBranch->m_oData.bRendered);

	CalcRenderVectors(pBranch);
	UpdateScreenSize(pBranch);

	float flDistance = pBranch->m_oData.flQuadDistance.GetUnits(SPGame()->GetSPRenderer()->GetRenderingScale());
	float flSize = pBranch->m_oData.flQuadRadius.GetUnits(SPGame()->GetSPRenderer()->GetRenderingScale());

	if (flDistance < 5 + flSize)
		return;

	if (flDistance > 5000 + flSize)
		return;

	m_apRenderBranches.push_back(pBranch);
	pBranch->m_oData.bRendered = true;
}

void CPlanetTerrain::Render(class CRenderingContext* c) const
{
	for (size_t i = 0; i < m_apRenderBranches.size(); i++)
		RenderBranch(m_apRenderBranches[i], c);
}

CVar r_showquaddebugoutlines("r_showquaddebugoutlines", "off");
CVar r_showquadnormals("r_showquadnormals", "off");

void CPlanetTerrain::RenderBranch(const CQuadTreeBranch<CBranchData>* pBranch, class CRenderingContext* c) const
{
	TAssert(pBranch->m_oData.bRendered);
	TAssert(!pBranch->m_oData.bRenderVectorsDirty);

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
	c->TexCoord(Vector2D(pBranch->m_vecMax.x, pBranch->m_vecMin.y));
	c->Normal(pBranch->m_oData.vec2n);
	c->Vertex(vec2);
	c->TexCoord(pBranch->m_vecMax);
	c->Normal(pBranch->m_oData.vec3n);
	c->Vertex(vec3);
	c->TexCoord(Vector2D(pBranch->m_vecMin.x, pBranch->m_vecMax.y));
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

void CPlanetTerrain::UpdateScreenSize(CQuadTreeBranch<CBranchData>* pBranch)
{
	if (pBranch->m_oData.flLastScreenUpdate < 0 || GameServer()->GetGameTime() - pBranch->m_oData.flLastScreenUpdate > r_terrainupdateinterval.GetFloat())
	{
		Vector vecUp;
		Vector vecForward;
		GameServer()->GetRenderer()->GetCameraVectors(&vecForward, NULL, &vecUp);

		CScalableMatrix mPlanet = m_pPlanet->GetGlobalScalableTransform();
		CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
		CScalableVector vecCharacterOrigin = pCharacter->GetGlobalScalableOrigin();

		CScalableVector vecQuadCenter = mPlanet * CScalableVector(pBranch->GetCenter(), m_pPlanet->GetScale()) - vecCharacterOrigin;
		CScalableVector vecQuadMax = mPlanet * CScalableVector(QuadTreeToWorld(this, pBranch->m_vecMax), m_pPlanet->GetScale()) - vecCharacterOrigin;

		scale_t eRenderScale = SPGame()->GetSPRenderer()->GetRenderingScale();

		Vector vecScreen = GameServer()->GetRenderer()->ScreenPosition(vecQuadCenter.GetUnits(eRenderScale));

		CScalableFloat flRadius = (vecQuadCenter-vecQuadMax).Length();
		Vector vecTop = GameServer()->GetRenderer()->ScreenPosition((vecQuadCenter + vecUp*flRadius).GetUnits(eRenderScale));

		CScalableVector vecPlayer = SPGame()->GetSPCamera()->GetCameraScalablePosition();

		float flWidth = (vecTop - vecScreen).Length()*2;
		CScalableFloat flDistance = (vecQuadCenter - vecPlayer).Length();

		pBranch->m_oData.flScreenSize = flWidth;
		pBranch->m_oData.flQuadDistance = flDistance;
		pBranch->m_oData.flQuadRadius = flRadius;
		pBranch->m_oData.flLastScreenUpdate = GameServer()->GetGameTime();
	}
}

CVar r_terrainresolution("r_terrainresolution", "1.0");
CVar r_minterrainsize("r_minterrainsize", "100");

bool CPlanetTerrain::ShouldRenderBranch(CQuadTreeBranch<CBranchData>* pBranch)
{
	CScalableVector vecQuadCenter(pBranch->GetCenter(), m_pPlanet->GetScale());
	CalcRenderVectors(pBranch);
	CScalableVector vecQuadMax(pBranch->m_oData.vec3, m_pPlanet->GetScale());

	CScalableFloat flRadius = (vecQuadCenter - vecQuadMax).Length();
	if (flRadius.GetUnits(SCALE_METER) < r_terrainresolution.GetFloat())
		return false;

	scale_t eScale = SPGame()->GetSPRenderer()->GetRenderingScale();

	CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
	CScalableVector vecCharacterOrigin = pCharacter->GetGlobalScalableOrigin();
	CScalableMatrix mPlanet = m_pPlanet->GetGlobalScalableTransform();

	CScalableVector vecPlanetCenter = mPlanet * vecQuadCenter - vecCharacterOrigin;

	if (!GameServer()->GetRenderer()->IsSphereInFrustum(vecPlanetCenter.GetUnits(eScale), flRadius.GetUnits(eScale)))
		return false;

	UpdateScreenSize(pBranch);

	if (pBranch->m_iDepth > m_pPlanet->GetMinQuadRenderDepth() && pBranch->m_oData.flScreenSize < r_minterrainsize.GetFloat())
		return false;

	return true;
}

void CPlanetTerrain::CalcRenderVectors(CQuadTreeBranch<CBranchData>* pBranch)
{
	if (!pBranch->m_oData.bRenderVectorsDirty)
		return;

	pBranch->m_oData.vec1 = m_pDataSource->QuadTreeToWorld(this, pBranch->m_vecMin);
	pBranch->m_oData.vec2 = m_pDataSource->QuadTreeToWorld(this, Vector2D(pBranch->m_vecMax.x, pBranch->m_vecMin.y));
	pBranch->m_oData.vec3 = m_pDataSource->QuadTreeToWorld(this, pBranch->m_vecMax);
	pBranch->m_oData.vec4 = m_pDataSource->QuadTreeToWorld(this, Vector2D(pBranch->m_vecMin.x, pBranch->m_vecMax.y));

	pBranch->m_oData.vec1n = pBranch->m_oData.vec1.Normalized();
	pBranch->m_oData.vec2n = pBranch->m_oData.vec2.Normalized();
	pBranch->m_oData.vec3n = pBranch->m_oData.vec3.Normalized();
	pBranch->m_oData.vec4n = pBranch->m_oData.vec4.Normalized();

	pBranch->m_oData.bRenderVectorsDirty = false;
}

Vector2D CPlanetTerrain::WorldToQuadTree(const CQuadTree<CBranchData>* pTree, const Vector& v) const
{
	TAssert(pTree == (CQuadTree<CBranchData>*)this);

	// Find the spot on the planet's surface that this point is closest to.
	Vector vecPlanetOrigin = m_pPlanet->GetGlobalOrigin();
	Vector vecPointDirection = (v - vecPlanetOrigin).Normalized();

	Vector vecDirection = GetDirection();

	if (vecPointDirection.Dot(vecDirection) <= 0)
		return Vector2D(-1, -1);

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

	float r = 1/vecPointDirection[iBig];

	float x = RemapVal(vecPointDirection[iX]*r, -1, 1, 0, 1);
	float y = RemapVal(vecPointDirection[iY]*r, -1, 1, 0, 1);

	return Vector2D(x, y);
}

Vector CPlanetTerrain::QuadTreeToWorld(const CQuadTree<CBranchData>* pTree, const Vector2D& v) const
{
	TAssert(pTree == (CQuadTree<CBranchData>*)this);

	Vector vecDirection = GetDirection();

	Vector vecCubePoint;

	float x = RemapVal(v.x, 0, 1, -1, 1);
	float y = RemapVal(v.y, 0, 1, -1, 1);

	if (vecDirection[0] != 0)
		vecCubePoint = Vector(vecDirection[0], x, y);
	else if (vecDirection[1] != 0)
		vecCubePoint = Vector(x, vecDirection[1], y);
	else
		vecCubePoint = Vector(x, y, vecDirection[2]);

	return vecCubePoint.Normalized() * m_pPlanet->GetRadius().GetUnits(m_pPlanet->GetScale());
}

Vector2D CPlanetTerrain::WorldToQuadTree(CQuadTree<CBranchData>* pTree, const Vector& v)
{
	return WorldToQuadTree((const CQuadTree<CBranchData>*)pTree, v);
}

Vector CPlanetTerrain::QuadTreeToWorld(CQuadTree<CBranchData>* pTree, const Vector2D& v)
{
	return QuadTreeToWorld((const CQuadTree<CBranchData>*)pTree, v);
}

bool CPlanetTerrain::ShouldBuildBranch(CQuadTreeBranch<CBranchData>* pBranch, bool& bDelete)
{
	bDelete = false;

	return ShouldRenderBranch(pBranch);
}
