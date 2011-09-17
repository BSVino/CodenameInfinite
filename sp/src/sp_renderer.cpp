#include "sp_renderer.h"

#include <GL/glew.h>

#include <mtrand.h>

#include <tinker/application.h>
#include <tinker/cvar.h>
#include <models/texturelibrary.h>
#include <game/gameserver.h>
#include <renderer/shaders.h>
#include <renderer/renderingcontext.h>
#include <tinker/profiler.h>

#include "sp_window.h"
#include "sp_game.h"
#include "sp_playercharacter.h"
#include "planet.h"
#include "star.h"
#include "sp_camera.h"

CSPRenderer::CSPRenderer()
	: CRenderer(CApplication::Get()->GetWindowWidth(), CApplication::Get()->GetWindowHeight())
{
	size_t iSkybox = CTextureLibrary::AddTextureID(_T("textures/skybox/skymap.png"), 2);
	SetSkybox(iSkybox, iSkybox, iSkybox, iSkybox, iSkybox, iSkybox);

	m_eRenderingScale = SCALE_NONE;
}

void CSPRenderer::LoadShaders()
{
	CShaderLibrary::AddShader("planet", "planet", "planet");
	CShaderLibrary::AddShader("skybox", "skybox", "skybox");
	CShaderLibrary::AddShader("brightpass", "pass", "brightpass");
	CShaderLibrary::AddShader("model", "pass", "model");
	CShaderLibrary::AddShader("blur", "pass", "blur");
}

void CSPRenderer::PreFrame()
{
	BaseClass::PreFrame();

	SPGame()->GetLocalPlayerCharacter()->LockViewToPlanet();
}

void CSPRenderer::BuildScaleFrustums()
{
	CSPCamera* pCamera = SPGame()->GetSPCamera();

	SetCameraUp(pCamera->GetCameraUp());
	SetCameraFOV(pCamera->GetCameraFOV());
	SetCameraNear(pCamera->GetCameraNear());
	SetCameraFar(pCamera->GetCameraFar());

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	// Build frustums for each render scale.
	for (size_t i = 0; i < SCALESTACK_SIZE; i++)
	{
		scale_t eScale = (scale_t)(i+1);
		m_eRenderingScale = eScale;

		SetCameraPosition(pCamera->GetCameraPosition().GetUnits(eScale));
		SetCameraTarget(pCamera->GetCameraTarget().GetUnits(eScale));

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		gluPerspective(
				m_flCameraFOV,
				(float)m_iWidth/(float)m_iHeight,
				m_flCameraNear,
				m_flCameraFar
			);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		gluLookAt(m_vecCameraPosition.x, m_vecCameraPosition.y, m_vecCameraPosition.z,
			m_vecCameraTarget.x, m_vecCameraTarget.y, m_vecCameraTarget.z,
			m_vecCameraUp.x, m_vecCameraUp.y, m_vecCameraUp.z);

		glGetDoublev( GL_MODELVIEW_MATRIX, m_aiScaleModelViews[i] );
		glGetDoublev( GL_PROJECTION_MATRIX, m_aiScaleProjections[i] );

		Matrix4x4 mModelView, mProjection;
		glGetFloatv( GL_MODELVIEW_MATRIX, mModelView );
		glGetFloatv( GL_PROJECTION_MATRIX, mProjection );

		m_aoScaleFrustums[i].CreateFrom(mModelView * mProjection);

		// Momentarily return the viewport to the window size. This is because if the scene buffer is not the same as the window size,
		// the viewport here will be the scene buffer size, but we need it to be the window size so we can do world/screen transformations.
		glPushAttrib(GL_VIEWPORT_BIT);
		glViewport(0, 0, (GLsizei)m_iWidth, (GLsizei)m_iHeight);
		glGetIntegerv( GL_VIEWPORT, m_aiScaleViewports[i] );
		glPopAttrib();
	}

	m_eRenderingScale = SCALE_NONE;

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

void CSPRenderer::StartRendering()
{
	TPROF("CSPRenderer::StartRendering");

	m_hClosestStar = NULL;
	m_ahRenderList.clear();

	BuildScaleFrustums();

	for (size_t i = 0; i < GameServer()->GetMaxEntities(); i++)
	{
		CBaseEntity* pEntity = CBaseEntity::GetEntity(i);
		if (pEntity && pEntity->ShouldRender())
			m_ahRenderList.push_back(pEntity);

		CPlanet* pPlanet = dynamic_cast<CPlanet*>(pEntity);
		if (pPlanet)
			pPlanet->RenderUpdate();

		CStar* pStar = dynamic_cast<CStar*>(pEntity);
		if (!pStar)
			continue;

		if (m_hClosestStar == NULL)
		{
			m_hClosestStar = pStar;
			continue;
		}

		if (pStar->GetGlobalOrigin().GetUnits(SCALE_METER).DistanceSqr(m_vecCameraPosition) < m_hClosestStar->GetGlobalOrigin().GetUnits(SCALE_METER).DistanceSqr(m_vecCameraPosition))
			m_hClosestStar = pStar;
	}

	RenderScale(SCALE_TERAMETER);
	RenderScale(SCALE_GIGAMETER);
	RenderScale(SCALE_MEGAMETER);
	RenderScale(SCALE_KILOMETER);
	RenderScale(SCALE_METER);
	RenderScale(SCALE_MILLIMETER);

	m_eRenderingScale = SCALE_RENDER;

	CCamera* pCamera = GameServer()->GetCamera();

	SetCameraPosition(pCamera->GetCameraPosition());
	SetCameraTarget(pCamera->GetCameraTarget());
	SetCameraUp(pCamera->GetCameraUp());
	SetCameraFOV(pCamera->GetCameraFOV());
	SetCameraNear(pCamera->GetCameraNear());
	SetCameraFar(pCamera->GetCameraFar());

	BaseClass::StartRendering();
}

CVar r_star_constant_attenuation("r_star_constant_attenuation", "0.1");
CVar r_star_linear_attenuation("r_star_linear_attenuation", "0.0");
CVar r_star_quadratic_attenuation("r_star_quadratic_attenuation", "0.0");

void CSPRenderer::SetupLighting()
{
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_COLOR_MATERIAL);
	glShadeModel(GL_SMOOTH);
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, Vector4D(Color(0, 0, 0)));

	if (m_hClosestStar == NULL)
		return;

	CStar* pStar = m_hClosestStar;
	CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();

	glLightfv(GL_LIGHT0, GL_POSITION, Vector4D(0, 0, 0, 1));
	glLightfv(GL_LIGHT0, GL_AMBIENT, Vector4D(Color(1, 2, 2)));
	glLightfv(GL_LIGHT0, GL_DIFFUSE, Vector4D(pStar->GetLightColor()));
	glLightfv(GL_LIGHT0, GL_SPECULAR, Vector4D(Color(15, 15, 15)));
	glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, r_star_constant_attenuation.GetFloat());
	glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, r_star_linear_attenuation.GetFloat());
	glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, r_star_quadratic_attenuation.GetFloat());
}

void CSPRenderer::DrawSkybox()
{
	m_eRenderingScale = SCALE_METER;
	BaseClass::DrawSkybox();
	m_eRenderingScale = SCALE_NONE;
}

void CSPRenderer::ModifySkyboxContext(CRenderingContext* c)
{
	c->UseProgram("skybox");

	CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
	if (!pCharacter)
		c->SetUniform("flAtmosphere", 0.0f);
	else
	{
		CPlanet* pPlanet = pCharacter->GetNearestPlanet();
		if (pPlanet)
		{
			CScalableFloat flDistance = (pPlanet->GetGlobalOrigin() - pCharacter->GetGlobalOrigin()).Length() - pPlanet->GetRadius();
			float flAtmosphere = (float)RemapVal(flDistance, CScalableFloat(1.0f, SCALE_KILOMETER), pPlanet->GetAtmosphereThickness(), 1, 0);
			c->SetUniform("flAtmosphere", flAtmosphere);
			c->SetUniform("vecUp", pCharacter->GetUpVector());
			c->SetUniform("clrSky", pPlanet->GetAtmosphereColor());
			if (GetClosestStar())
			{
				c->SetUniform("vecStar", GetClosestStar()->GameData().GetScalableRenderOrigin().GetUnits(SCALE_METER).Normalized());
				c->SetUniform("clrStar", GetClosestStar()->GetLightColor());
			}
		}
		else
			c->SetUniform("flAtmosphere", 0.0f);
	}
}

void CSPRenderer::FinishRendering()
{
	TPROF("CSPRenderer::FinishRendering");

	BaseClass::FinishRendering();

	m_eRenderingScale = SCALE_NONE;
}

CVar r_renderscale("r_renderscale", "0");

void CSPRenderer::RenderScale(scale_t eRenderScale)
{
	m_eRenderingScale = eRenderScale;

	size_t iEntities = m_ahRenderList.size();
	if (iEntities == 0)
		return;

	CSPCamera* pCamera = SPGame()->GetSPCamera();

	SetCameraPosition(pCamera->GetCameraPosition().GetUnits(m_eRenderingScale));
	SetCameraTarget(pCamera->GetCameraTarget().GetUnits(m_eRenderingScale));
	SetCameraUp(pCamera->GetCameraUp());
	SetCameraFOV(pCamera->GetCameraFOV());
	SetCameraNear(pCamera->GetCameraNear());
	SetCameraFar(pCamera->GetCameraFar());

	BaseClass::StartRendering();

	glClear(GL_DEPTH_BUFFER_BIT);

	SetupLighting();

	bool bFrustumCulling = CVar::GetCVarBool("r_frustumculling");

	eastl::vector<CBaseEntity*> apRender;

	// First render all opaque objects
	for (size_t i = 0; i < iEntities; i++)
	{
		CBaseEntity* pEntity = m_ahRenderList[i];

		if (!pEntity->GameData().ShouldRenderAtScale(m_eRenderingScale))
			continue;

		if (bFrustumCulling && !IsSphereInFrustum(pEntity->GameData().GetScalableRenderOrigin().GetUnits(m_eRenderingScale), (float)pEntity->GetRenderRadius().GetUnits(m_eRenderingScale)))
			continue;

		apRender.push_back(pEntity);
	}

	size_t iRenderEntities = apRender.size();

	if (r_renderscale.GetInt() && r_renderscale.GetInt() != eRenderScale)
		iRenderEntities = 0;

	for (size_t i = 0; i < iRenderEntities; i++)
		apRender[i]->Render(false);

	for (size_t i = 0; i < iRenderEntities; i++)
		apRender[i]->Render(true);

	BaseClass::FinishRendering();
}

bool CSPRenderer::IsInFrustumAtScale(scale_t eRenderScale, const Vector& vecCenter, float flRadius)
{
	return m_aoScaleFrustums[eRenderScale-1].TouchesSphere(vecCenter, flRadius);
}

bool CSPRenderer::IsInFrustumAtScaleSidesOnly(scale_t eRenderScale, const Vector& vecCenter, float flRadius)
{
	return m_aoScaleFrustums[eRenderScale-1].TouchesSphereSidesOnly(vecCenter, flRadius);
}

bool CSPRenderer::FrustumContainsAtScaleSidesOnly(scale_t eRenderScale, const Vector& vecCenter, float flRadius)
{
	return m_aoScaleFrustums[eRenderScale-1].ContainsSphereSidesOnly(vecCenter, flRadius);
}

Vector CSPRenderer::ScreenPositionAtScale(scale_t eRenderScale, const Vector& vecWorld)
{
	GLdouble x, y, z;
	gluProject(
		vecWorld.x, vecWorld.y, vecWorld.z,
		(GLdouble*)m_aiScaleModelViews[eRenderScale], (GLdouble*)m_aiScaleProjections[eRenderScale], (GLint*)m_aiScaleViewports[eRenderScale],
		&x, &y, &z);
	return Vector((float)x, (float)m_iHeight - (float)y, (float)z);
}

Vector CSPRenderer::WorldPositionAtScale(scale_t eRenderScale, const Vector& vecScreen)
{
	GLdouble x, y, z;
	gluUnProject(
		vecScreen.x, (float)m_iHeight - vecScreen.y, vecScreen.z,
		(GLdouble*)m_aiScaleModelViews[eRenderScale], (GLdouble*)m_aiScaleProjections[eRenderScale], (GLint*)m_aiScaleViewports[eRenderScale],
		&x, &y, &z);
	return Vector((float)x, (float)y, (float)z);
}

CStar* CSPRenderer::GetClosestStar() const
{
	return m_hClosestStar;
}
