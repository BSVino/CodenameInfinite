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
	m_iSkyboxFT = CTextureLibrary::AddTextureID(_T("textures/skybox/skymap.png"), 2);
	m_iSkyboxLF = CTextureLibrary::AddTextureID(_T("textures/skybox/skymap.png"), 2);
	m_iSkyboxBK = CTextureLibrary::AddTextureID(_T("textures/skybox/skymap.png"), 2);
	m_iSkyboxRT = CTextureLibrary::AddTextureID(_T("textures/skybox/skymap.png"), 2);
	m_iSkyboxDN = CTextureLibrary::AddTextureID(_T("textures/skybox/skymap.png"), 2);
	m_iSkyboxUP = CTextureLibrary::AddTextureID(_T("textures/skybox/skymap.png"), 2);

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
	for (size_t i = SCALE_NONE; i <= SCALE_HIGHEST; i++)
		m_ahRenderScales[i].clear();

	for (size_t i = 0; i < GameServer()->GetMaxEntities(); i++)
	{
		CBaseEntity* pEntity = CBaseEntity::GetEntity(i);
		CSPEntity* pSPEntity = dynamic_cast<CSPEntity*>(pEntity);
		if (pSPEntity)
		{
			scale_t eScale = pSPEntity->GetScale();
			m_ahRenderScales[eScale].push_back(pSPEntity);
		}

		CStar* pStar = dynamic_cast<CStar*>(pEntity);
		if (!pStar)
			continue;

		if (m_hClosestStar == NULL)
		{
			m_hClosestStar = pStar;
			continue;
		}

		if (pStar->GetGlobalOrigin().DistanceSqr(m_vecCameraPosition) < m_hClosestStar->GetGlobalOrigin().DistanceSqr(m_vecCameraPosition))
			m_hClosestStar = pStar;
	}

	RenderSkybox();

	RenderScale(SCALE_LIGHTYEAR);
	RenderScale(SCALE_AU);
	RenderScale(SCALE_MEGAMETER);
	RenderScale(SCALE_KILOMETER);

	m_eRenderingScale = SCALE_METER;

	SetCameraPosition(GameServer()->GetCamera()->GetCameraPosition());
	SetCameraTarget(GameServer()->GetCamera()->GetCameraTarget());
	SetCameraUp(GameServer()->GetCamera()->GetCameraUp());
	SetCameraFOV(GameServer()->GetCamera()->GetCameraFOV());
	SetCameraNear(GameServer()->GetCamera()->GetCameraNear());
	SetCameraFar(GameServer()->GetCamera()->GetCameraFar());

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

	for (size_t i = 0; i < GameServer()->GetMaxEntities(); i++)
	{
		CStar* pStar = dynamic_cast<CStar*>(CBaseEntity::GetEntity(i));
		if (!pStar)
			continue;

		glLightfv(GL_LIGHT0, GL_POSITION, Vector4D(CScalableVector(pStar->GetGlobalOrigin(), pStar->GetScale()).GetUnits(m_eRenderingScale)) + Vector4D(0,0,0,1));
		glLightfv(GL_LIGHT0, GL_AMBIENT, Vector4D(Color(1, 2, 2)));
		glLightfv(GL_LIGHT0, GL_DIFFUSE, Vector4D(Color(255, 242, 143)));
		glLightfv(GL_LIGHT0, GL_SPECULAR, Vector4D(Color(15, 15, 15)));
		glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, r_star_constant_attenuation.GetFloat());
		glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, r_star_linear_attenuation.GetFloat());
		glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, r_star_quadratic_attenuation.GetFloat());
		break;
	}
}

void CSPRenderer::RenderSkybox()
{
	if (!SPGame())
		return;

	TPROF("CSPRenderer::RenderSkybox");

	m_eRenderingScale = SCALE_METER;

	CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();

	SetCameraPosition(Vector(0, 0, 0));
	SetCameraTarget(Vector(pCharacter->GetGlobalTransform().GetColumn(0)));
	SetCameraUp(GameServer()->GetCamera()->GetCameraUp());
	SetCameraFOV(GameServer()->GetCamera()->GetCameraFOV());
	SetCameraNear(GameServer()->GetCamera()->GetCameraNear());
	SetCameraFar(GameServer()->GetCamera()->GetCameraFar());

	glPushAttrib(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_ENABLE_BIT|GL_TEXTURE_BIT|GL_CURRENT_BIT);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();

	gluPerspective(
			m_flCameraFOV,
			(float)m_iWidth/(float)m_iHeight,
			m_flCameraNear,
			m_flCameraFar
		);

	glMatrixMode(GL_MODELVIEW);

	glPushMatrix();
	glLoadIdentity();

	gluLookAt(m_vecCameraPosition.x, m_vecCameraPosition.y, m_vecCameraPosition.z,
		m_vecCameraTarget.x, m_vecCameraTarget.y, m_vecCameraTarget.z,
		m_vecCameraUp.x, m_vecCameraUp.y, m_vecCameraUp.z);

	glEnable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_2D);

	if (true)
	{
		glPushAttrib(GL_CURRENT_BIT|GL_ENABLE_BIT|GL_TEXTURE_BIT);
		glPushMatrix();
		glTranslatef(m_vecCameraPosition.x, m_vecCameraPosition.y, m_vecCameraPosition.z);

		glDisable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);

		if (GLEW_ARB_multitexture || GLEW_VERSION_1_3)
			glActiveTexture(GL_TEXTURE0);
		glEnable(GL_TEXTURE_2D);

		glBindTexture(GL_TEXTURE_2D, (GLuint)m_iSkyboxFT);
		glBegin(GL_QUADS);
			glTexCoord2i(0, 1); glVertex3f(100, 100, -100);
			glTexCoord2i(0, 0); glVertex3f(100, -100, -100);
			glTexCoord2i(1, 0); glVertex3f(100, -100, 100);
			glTexCoord2i(1, 1); glVertex3f(100, 100, 100);
		glEnd();

		glBindTexture(GL_TEXTURE_2D, (GLuint)m_iSkyboxLF);
		glBegin(GL_QUADS);
			glTexCoord2i(0, 1); glVertex3f(-100, 100, -100);
			glTexCoord2i(0, 0); glVertex3f(-100, -100, -100);
			glTexCoord2i(1, 0); glVertex3f(100, -100, -100);
			glTexCoord2i(1, 1); glVertex3f(100, 100, -100);
		glEnd();

		glBindTexture(GL_TEXTURE_2D, (GLuint)m_iSkyboxBK);
		glBegin(GL_QUADS);
			glTexCoord2i(0, 1); glVertex3f(-100, 100, 100);
			glTexCoord2i(0, 0); glVertex3f(-100, -100, 100);
			glTexCoord2i(1, 0); glVertex3f(-100, -100, -100);
			glTexCoord2i(1, 1); glVertex3f(-100, 100, -100);
		glEnd();

		glBindTexture(GL_TEXTURE_2D, (GLuint)m_iSkyboxRT);
		glBegin(GL_QUADS);
			glTexCoord2i(0, 1); glVertex3f(100, 100, 100);
			glTexCoord2i(0, 0); glVertex3f(100, -100, 100);
			glTexCoord2i(1, 0); glVertex3f(-100, -100, 100);
			glTexCoord2i(1, 1); glVertex3f(-100, 100, 100);
		glEnd();

		glBindTexture(GL_TEXTURE_2D, (GLuint)m_iSkyboxUP);
		glBegin(GL_QUADS);
			glTexCoord2i(0, 1); glVertex3f(-100, 100, -100);
			glTexCoord2i(0, 0); glVertex3f(100, 100, -100);
			glTexCoord2i(1, 0); glVertex3f(100, 100, 100);
			glTexCoord2i(1, 1); glVertex3f(-100, 100, 100);
		glEnd();

		glBindTexture(GL_TEXTURE_2D, (GLuint)m_iSkyboxDN);
		glBegin(GL_QUADS);
			glTexCoord2i(0, 1); glVertex3f(100, -100, -100);
			glTexCoord2i(0, 0); glVertex3f(-100, -100, -100);
			glTexCoord2i(1, 0); glVertex3f(-100, -100, 100);
			glTexCoord2i(1, 1); glVertex3f(100, -100, 100);
		glEnd();

		glPopMatrix();
		glPopAttrib();
	}

	glClear(GL_DEPTH_BUFFER_BIT);

	glPopMatrix();
	glPopAttrib();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
}

void CSPRenderer::FinishRendering()
{
	TPROF("CSPRenderer::FinishRendering");

	BaseClass::FinishRendering();

	m_eRenderingScale = SCALE_NONE;
}

void CSPRenderer::RenderScale(scale_t eRenderScale)
{
	m_eRenderingScale = eRenderScale;

	eastl::vector<CEntityHandle<CSPEntity> >& ahRender = m_ahRenderScales[m_eRenderingScale];
	size_t iEntities = ahRender.size();
	if (iEntities == 0)
		return;

	SetCameraPosition(GameServer()->GetCamera()->GetCameraPosition());
	SetCameraTarget(GameServer()->GetCamera()->GetCameraTarget());
	SetCameraUp(GameServer()->GetCamera()->GetCameraUp());
	SetCameraFOV(GameServer()->GetCamera()->GetCameraFOV());
	SetCameraNear(GameServer()->GetCamera()->GetCameraNear());
	SetCameraFar(GameServer()->GetCamera()->GetCameraFar());

	BaseClass::StartRendering();

	SetupLighting();

	bool bFrustumCulling = CVar::GetCVarBool("r_frustumculling");

	// First render all opaque objects
	for (size_t i = 0; i < iEntities; i++)
	{
		CSPEntity* pSPEntity = ahRender[i];
		if (bFrustumCulling && !IsSphereInFrustum(CScalableVector(pSPEntity->GetRenderOrigin(), pSPEntity->GetScale()).GetUnits(m_eRenderingScale), CScalableFloat(pSPEntity->GetRenderRadius(), pSPEntity->GetScale()).GetUnits(m_eRenderingScale)))
			continue;

		CPlanet* pPlanet = dynamic_cast<CPlanet*>(ahRender[i].GetPointer());
		if (pPlanet)
			pPlanet->RenderUpdate();

		ahRender[i]->Render(false);
	}

	for (size_t i = 0; i < iEntities; i++)
		ahRender[i]->Render(true);

	BaseClass::FinishRendering();
}
