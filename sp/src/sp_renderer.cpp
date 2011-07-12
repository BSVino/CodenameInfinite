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

CSPRenderer::CSPRenderer()
	: CRenderer(CApplication::Get()->GetWindowWidth(), CApplication::Get()->GetWindowHeight())
{
	m_iSkyboxFT = CTextureLibrary::AddTextureID(_T("textures/skybox/standard-ft.png"), 2);
	m_iSkyboxLF = CTextureLibrary::AddTextureID(_T("textures/skybox/standard-lf.png"), 2);
	m_iSkyboxBK = CTextureLibrary::AddTextureID(_T("textures/skybox/standard-bk.png"), 2);
	m_iSkyboxRT = CTextureLibrary::AddTextureID(_T("textures/skybox/standard-rt.png"), 2);
	m_iSkyboxDN = CTextureLibrary::AddTextureID(_T("textures/skybox/standard-dn.png"), 2);
	m_iSkyboxUP = CTextureLibrary::AddTextureID(_T("textures/skybox/standard-up.png"), 2);
}

void CSPRenderer::StartRendering()
{
	TPROF("CSPRenderer::StartRendering");

	BaseClass::StartRendering();

	RenderSkybox();

	RenderGround();
}

void CSPRenderer::RenderSkybox()
{
	if (!SPGame())
		return;

	TPROF("CSPRenderer::RenderSkybox");

	if (true)
	{
		glPushAttrib(GL_CURRENT_BIT|GL_ENABLE_BIT);
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

void CSPRenderer::RenderGround(void)
{
#ifdef _DEBUG
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);

	glPushMatrix();

	int i;

	for (i = 0; i < 20; i++)
	{
		Vector vecStartX(-100, 0, -100);
		Vector vecEndX(-100, 0, 100);
		Vector vecStartZ(-100, 0, -100);
		Vector vecEndZ(100, 0, -100);

		for (int j = 0; j <= 20; j++)
		{
			GLfloat aflBorderLineBright[3] = { 0.7f, 0.7f, 0.7f };
			GLfloat aflBorderLineDarker[3] = { 0.6f, 0.6f, 0.6f };
			GLfloat aflInsideLineBright[3] = { 0.5f, 0.5f, 0.5f };
			GLfloat aflInsideLineDarker[3] = { 0.4f, 0.4f, 0.4f };

			glBegin(GL_LINES);

				if (j == 0 || j == 20 || j == 10)
					glColor3fv(aflBorderLineBright);
				else
					glColor3fv(aflInsideLineBright);

				glVertex3fv(vecStartX);

				if (j == 0 || j == 20 || j == 10)
					glColor3fv(aflBorderLineDarker);
				else
					glColor3fv(aflInsideLineDarker);

				if (j == 10)
					glVertex3fv(Vector(0, 0, 0));
				else
					glVertex3fv(vecEndX);

				if (j == 10)
				{
					glColor3f(0.7f, 0.2f, 0.2f);
					glVertex3fv(Vector(0, 0, 0));
					glVertex3fv(Vector(100, 0, 0));
				}

			glEnd();

			glBegin(GL_LINES);

				if (j == 0 || j == 20 || j == 10)
					glColor3fv(aflBorderLineBright);
				else
					glColor3fv(aflInsideLineBright);

				glVertex3fv(vecStartZ);

				if (j == 0 || j == 20 || j == 10)
					glColor3fv(aflBorderLineDarker);
				else
					glColor3fv(aflInsideLineDarker);

				if (j == 10)
					glVertex3fv(Vector(0, 0, 0));
				else
					glVertex3fv(vecEndZ);

				if (j == 10)
				{
					glColor3f(0.2f, 0.2f, 0.7f);
					glVertex3fv(Vector(0, 0, 0));
					glVertex3fv(Vector(0, 0, 100));
				}

			glEnd();

			vecStartX.x += 10;
			vecEndX.x += 10;
			vecStartZ.z += 10;
			vecEndZ.z += 10;
		}
	}

	glPopMatrix();
#endif
}
