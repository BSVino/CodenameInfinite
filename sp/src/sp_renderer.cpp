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

CSPRenderer::CSPRenderer()
	: CRenderer(CApplication::Get()->GetWindowWidth(), CApplication::Get()->GetWindowHeight())
{
	m_iSkyboxFT = CTextureLibrary::AddTextureID(_T("textures/skybox/skymap.png"), 2);
	m_iSkyboxLF = CTextureLibrary::AddTextureID(_T("textures/skybox/skymap.png"), 2);
	m_iSkyboxBK = CTextureLibrary::AddTextureID(_T("textures/skybox/skymap.png"), 2);
	m_iSkyboxRT = CTextureLibrary::AddTextureID(_T("textures/skybox/skymap.png"), 2);
	m_iSkyboxDN = CTextureLibrary::AddTextureID(_T("textures/skybox/skymap.png"), 2);
	m_iSkyboxUP = CTextureLibrary::AddTextureID(_T("textures/skybox/skymap.png"), 2);
}

void CSPRenderer::PreFrame()
{
	BaseClass::PreFrame();

	SPGame()->GetLocalPlayerCharacter()->LockViewToPlanet();

	m_ahPlanetsToUpdate.clear();
}

void CSPRenderer::StartRendering()
{
	TPROF("CSPRenderer::StartRendering");

	BaseClass::StartRendering();

	// Now is the time to have the terrain calculate its junk. The camera and screen/world space stuff are set up.
	for (size_t i = 0; i < m_ahPlanetsToUpdate.size(); i++)
	{
		CPlanet* pPlanet = m_ahPlanetsToUpdate[i];
		if (!pPlanet)
			continue;

		pPlanet->RenderUpdate();
	}

	RenderSkybox();
}

void CSPRenderer::RenderSkybox()
{
	if (!SPGame())
		return;

	TPROF("CSPRenderer::RenderSkybox");

	if (true)
	{
		glPushAttrib(GL_CURRENT_BIT|GL_ENABLE_BIT|GL_TEXTURE_BIT);
		glPushMatrix();
		glTranslatef(m_vecCameraPosition.x, m_vecCameraPosition.y, m_vecCameraPosition.z);

		glDisable(GL_DEPTH_TEST);

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

	// Set camera 1/16 to match the scale of the skybox
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(m_vecCameraPosition.x/16, m_vecCameraPosition.y/16, m_vecCameraPosition.z/16,
		m_vecCameraTarget.x/16, m_vecCameraTarget.y/16, m_vecCameraTarget.z/16,
		m_vecCameraUp.x, m_vecCameraUp.y, m_vecCameraUp.z);

	// Draw skybox shit.

	// Reset the camera
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(m_vecCameraPosition.x, m_vecCameraPosition.y, m_vecCameraPosition.z,
		m_vecCameraTarget.x, m_vecCameraTarget.y, m_vecCameraTarget.z,
		m_vecCameraUp.x, m_vecCameraUp.y, m_vecCameraUp.z);

	glClear(GL_DEPTH_BUFFER_BIT);
}
void CSPRenderer::AddPlanetToUpdate(CPlanet* pPlanet)
{
	m_ahPlanetsToUpdate.push_back(pPlanet);
}
