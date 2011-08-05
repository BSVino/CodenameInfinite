#include "sp_renderer.h"

#include <GL/glew.h>

#include <mtrand.h>

#include <tinker/application.h>
#include <tinker/cvar.h>
#include <models/texturelibrary.h>
#include <game/gameserver.h>
#include <shaders/shaders.h>
#include <tinker/profiler.h>

#include "sp_window.h"
#include "sp_game.h"
#include "sp_character.h"
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

void CSPRenderer::PreFrame()
{
	BaseClass::PreFrame();

	SPGame()->GetLocalPlayerCharacter()->LockViewToPlanet();
}

CVar r_star_constant_attenuation("r_star_constant_attenuation", "0.1");
CVar r_star_linear_attenuation("r_star_linear_attenuation", "0.0");
CVar r_star_quadratic_attenuation("r_star_quadratic_attenuation", "0.0");

void CSPRenderer::StartRendering()
{
	TPROF("CSPRenderer::StartRendering");

	m_hClosestStar = NULL;
	m_ahRenderList.clear();

	for (size_t i = 0; i < GameServer()->GetMaxEntities(); i++)
	{
		CBaseEntity* pEntity = CBaseEntity::GetEntity(i);
		CSPEntity* pSPEntity = dynamic_cast<CSPEntity*>(pEntity);
		if (pSPEntity)
			m_ahRenderList.push_back(pSPEntity);

		CStar* pStar = dynamic_cast<CStar*>(pEntity);
		if (!pStar)
			continue;

		if (m_hClosestStar == NULL)
		{
			m_hClosestStar = pStar;
			continue;
		}

		if (pStar->GetGlobalScalableOrigin().GetUnits(SCALE_METER).DistanceSqr(m_vecCameraPosition) < m_hClosestStar->GetGlobalScalableOrigin().GetUnits(SCALE_METER).DistanceSqr(m_vecCameraPosition))
			m_hClosestStar = pStar;
	}

	RenderScale(SCALE_TERAMETER);
	RenderScale(SCALE_GIGAMETER);
	RenderScale(SCALE_MEGAMETER);
	RenderScale(SCALE_KILOMETER);
	RenderScale(SCALE_METER);

	m_eRenderingScale = SCALE_METER;

	CCamera* pCamera = GameServer()->GetCamera();

	SetCameraPosition(pCamera->GetCameraPosition());
	SetCameraTarget(pCamera->GetCameraTarget());
	SetCameraUp(pCamera->GetCameraUp());
	SetCameraFOV(pCamera->GetCameraFOV());
	SetCameraNear(pCamera->GetCameraNear());
	SetCameraFar(pCamera->GetCameraFar());

	BaseClass::StartRendering();
}

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

	glLightfv(GL_LIGHT0, GL_POSITION, Vector4D(pStar->GetScalableRenderOrigin().GetUnits(m_eRenderingScale)) + Vector4D(0,0,0,1));
	glLightfv(GL_LIGHT0, GL_AMBIENT, Vector4D(Color(1, 2, 2)));
	glLightfv(GL_LIGHT0, GL_DIFFUSE, Vector4D(Color(255, 242, 143)));
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

	SetCameraPosition(pCamera->GetCameraScalablePosition().GetUnits(m_eRenderingScale));
	SetCameraTarget(pCamera->GetCameraScalableTarget().GetUnits(m_eRenderingScale));
	SetCameraUp(pCamera->GetCameraUp());
	SetCameraFOV(pCamera->GetCameraFOV());
	SetCameraNear(pCamera->GetCameraNear());
	SetCameraFar(pCamera->GetCameraFar());

	BaseClass::StartRendering();

	glClear(GL_DEPTH_BUFFER_BIT);

	SetupLighting();

	bool bFrustumCulling = CVar::GetCVarBool("r_frustumculling");

	eastl::vector<CSPEntity*> apRender;

	// First render all opaque objects
	for (size_t i = 0; i < iEntities; i++)
	{
		CSPEntity* pSPEntity = m_ahRenderList[i];

		if (!pSPEntity->ShouldRenderAtScale(m_eRenderingScale))
			continue;

		if (bFrustumCulling && !IsSphereInFrustum(pSPEntity->GetScalableRenderOrigin().GetUnits(m_eRenderingScale), pSPEntity->GetScalableRenderRadius().GetUnits(m_eRenderingScale)))
			continue;

		CPlanet* pPlanet = dynamic_cast<CPlanet*>(pSPEntity);
		if (pPlanet)
			pPlanet->RenderUpdate();

		apRender.push_back(pSPEntity);
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
