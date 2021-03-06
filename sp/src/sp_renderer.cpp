#include "sp_renderer.h"

#include <GL3/gl3w.h>
#include <GL/glu.h>

#include <mtrand.h>

#include <tinker/application.h>
#include <tinker/cvar.h>
#include <textures/materiallibrary.h>
#include <textures/texturelibrary.h>
#include <game/gameserver.h>
#include <renderer/shaders.h>
#include <renderer/renderingcontext.h>
#include <tinker/profiler.h>
#include <tengine/game/cameramanager.h>

#include "sp_window.h"
#include "entities/sp_game.h"
#include "entities/sp_playercharacter.h"
#include "planet/planet.h"
#include "entities/star.h"
#include "entities/sp_camera.h"

CSPRenderer::CSPRenderer()
	: CGameRenderer(CApplication::Get()->GetWindowWidth(), CApplication::Get()->GetWindowHeight())
{
	CTextureHandle hSkybox("textures/skybox/skymap.png");

	SetSkybox(hSkybox, hSkybox, hSkybox, hSkybox, hSkybox, hSkybox);

	m_eRenderingScale = SCALE_NONE;
}

void CSPRenderer::Initialize()
{
	BaseClass::Initialize();

	m_oCommandMenuBuffer = CreateFrameBuffer(512, 512, (fb_options_e)(FB_TEXTURE));
}

void CSPRenderer::PreFrame()
{
	BaseClass::PreFrame();

	SPGame()->GetLocalPlayerCharacter()->LockViewToPlanet();
}

void CSPRenderer::BuildScaleFrustums()
{
	CCameraManager* pCameraManager = GameServer()->GetCameraManager();

	SetCameraUp(pCameraManager->GetCameraUp());
	SetCameraFOV(pCameraManager->GetCameraFOV());
	SetCameraNear(pCameraManager->GetCameraNear());
	SetCameraFar(pCameraManager->GetCameraFar());
	SetCameraDirection(pCameraManager->GetCameraDirection());

	Matrix4x4 mProjection = Matrix4x4::ProjectPerspective(
			m_flCameraFOV,
			(float)m_iWidth/(float)m_iHeight,
			m_flCameraNear,
			m_flCameraFar
		);

	// Build frustums for each render scale.
	for (size_t i = 0; i < SCALESTACK_SIZE; i++)
	{
		scale_t eScale = (scale_t)(i+1);
		m_eRenderingScale = eScale;

		SetCameraPosition(pCameraManager->GetCameraPosition().GetUnits(eScale));

		Matrix4x4 mView = Matrix4x4::ConstructCameraView(m_vecCameraPosition, m_vecCameraDirection, m_vecCameraUp);

		for (size_t x = 0; x < 16; x++)
		{
			m_aiScaleModelViews[i][x] = ((float*)mView)[x];
			m_aiScaleProjections[i][x] = ((float*)mProjection)[x];
		}

		m_aoScaleFrustums[i].CreateFrom(mProjection * mView);

		glGetIntegerv( GL_VIEWPORT, m_aiScaleViewports[i] );
	}

	m_eRenderingScale = SCALE_NONE;
}

void CSPRenderer::StartRendering(class CRenderingContext* pContext)
{
	TPROF("CSPRenderer::StartRendering");

	m_hClosestStar = NULL;
	m_ahRenderList.clear();

	BuildScaleFrustums();

	CPlayerCharacter* pLocalPlayer = SPGame()->GetLocalPlayerCharacter();

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

		if (!m_hClosestStar)
		{
			m_hClosestStar = pStar;
			continue;
		}

		if ((pStar->GetGlobalOrigin()-pLocalPlayer->GetGlobalOrigin()).LengthSqr() < (m_hClosestStar->GetGlobalOrigin()-pLocalPlayer->GetGlobalOrigin()).LengthSqr())
			m_hClosestStar = pStar;
	}

	RenderScale(SCALE_TERAMETER, pContext);
	RenderScale(SCALE_GIGAMETER, pContext);
	RenderScale(SCALE_MEGAMETER, pContext);
	RenderScale(SCALE_KILOMETER, pContext);
	RenderScale(SCALE_METER, pContext);

	m_eRenderingScale = SCALE_RENDER;

	CCameraManager* pCamera = GameServer()->GetCameraManager();

	SetCameraPosition(pCamera->GetCameraPosition());
	SetCameraDirection(pCamera->GetCameraDirection());
	SetCameraUp(pCamera->GetCameraUp());
	SetCameraFOV(pCamera->GetCameraFOV());
	SetCameraNear(pCamera->GetCameraNear());
	SetCameraFar(pCamera->GetCameraFar());
	SetCustomProjection(pCamera->UseCustomProjection());
	SetCustomProjection(pCamera->GetCustomProjection());

	BaseClass::StartRendering(pContext);
}

CVar r_star_constant_attenuation("r_star_constant_attenuation", "0.1");
CVar r_star_linear_attenuation("r_star_linear_attenuation", "0.0");
CVar r_star_quadratic_attenuation("r_star_quadratic_attenuation", "0.0");

void CSPRenderer::DrawSkybox(CRenderingContext* c)
{
	m_eRenderingScale = SCALE_METER;
	BaseClass::DrawSkybox(c);
	m_eRenderingScale = SCALE_NONE;
}

void CSPRenderer::ModifySkyboxContext(CRenderingContext* c)
{
	CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
	if (!pCharacter)
		c->SetUniform("flAtmosphere", 0.0f);
	else
	{
		CPlanet* pPlanet = pCharacter->GetNearestPlanet();
		if (pPlanet)
		{
			Matrix4x4 mToPlanetLocal = pPlanet->GetGlobalToLocalTransform().GetMeters() * Matrix4x4();
			mToPlanetLocal.SetTranslation(Vector());

			// The skybox is drawn globally even if we're in planet-local rendering mode.
			c->ResetTransformations();
			c->Transform(mToPlanetLocal);

			CScalableFloat flDistance = (pPlanet->GetGlobalOrigin() - pCharacter->GetGlobalOrigin()).Length() - pPlanet->GetRadius();
			float flAtmosphere = (float)RemapVal(flDistance, CScalableFloat(1.0f, SCALE_KILOMETER), pPlanet->GetAtmosphereThickness(), 1, 0);
			c->SetUniform("flAtmosphere", flAtmosphere);
			c->SetUniform("vecUp", pCharacter->GetUpVector());
			c->SetUniform("clrSky", Vector(pPlanet->GetAtmosphereColor()));
			if (GetClosestStar())
			{
				c->SetUniform("vecStar", (GetClosestStar()->GetGlobalOrigin()-pPlanet->GetGlobalOrigin()).GetMeters().Normalized());
				c->SetUniform("clrStar", Vector(GetClosestStar()->GetLightColor()));
			}
		}
		else
			c->SetUniform("flAtmosphere", 0.0f);
	}
}

void CSPRenderer::FinishRendering(CRenderingContext* c)
{
	BaseClass::FinishRendering(c);

	m_eRenderingScale = SCALE_NONE;
}

CVar r_renderscale("r_renderscale", "0");

void CSPRenderer::RenderScale(scale_t eRenderScale, CRenderingContext* pContext)
{
	m_eRenderingScale = eRenderScale;

	size_t iEntities = m_ahRenderList.size();
	if (iEntities == 0)
		return;

	CCameraManager* pCamera = GameServer()->GetCameraManager();

	SetCameraPosition(pCamera->GetCameraPosition().GetUnits(m_eRenderingScale));
	SetCameraDirection(pCamera->GetCameraDirection());
	SetCameraUp(pCamera->GetCameraUp());
	SetCameraFOV(pCamera->GetCameraFOV());
	SetCameraNear(pCamera->GetCameraNear());
	SetCameraFar(pCamera->GetCameraFar());

	BaseClass::StartRendering(pContext);

	pContext->ClearDepth();

	if (!r_renderscale.GetBool() || r_renderscale.GetInt() == eRenderScale)
		RenderEverything();

	BaseClass::FinishRendering(pContext);
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

bool CSPRenderer::ShouldRenderParticles() const
{
	return m_eRenderingScale == SCALE_RENDER;
}
