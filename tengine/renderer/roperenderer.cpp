#include "roperenderer.h"

#include <GL/glew.h>
#include <IL/il.h>
#include <IL/ilu.h>

#include <maths.h>
#include <simplex.h>

#include <modelconverter/convmesh.h>
#include <models/models.h>
#include <renderer/shaders.h>
#include <tinker/application.h>
#include <tinker/cvar.h>
#include <tinker/profiler.h>
#include <game/gameserver.h>
#include <models/texturelibrary.h>
#include <renderer/renderer.h>

CRopeRenderer::CRopeRenderer(CRenderer *pRenderer, size_t iTexture, Vector vecStart, float flWidth)
	: m_oContext(pRenderer)
{
	m_pRenderer = pRenderer;

	m_oContext.BindTexture(iTexture);
	m_vecLastLink = vecStart;
	m_bFirstLink = true;

	m_flWidth = m_flLastLinkWidth = flWidth;

	m_flTextureScale = 1;
	m_flTextureOffset = 0;

	m_bUseForward = false;

	m_oContext.SetBlend(BLEND_ADDITIVE);
	m_oContext.SetDepthMask(false);

	m_clrRope = Color(255, 255, 255, 255);
	m_oContext.BeginRenderQuads();
}

void CRopeRenderer::AddLink(Vector vecLink)
{
	Vector vecForward;
	if (m_bUseForward)
		vecForward = m_vecForward;
	else
		vecForward = m_pRenderer->GetCameraVector();

	Vector vecUp = (vecLink - m_vecLastLink).Normalized();
	Vector vecLastRight = vecForward.Cross(vecUp)*(m_flLastLinkWidth/2);
	Vector vecRight = vecForward.Cross(vecUp)*(m_flWidth/2);

	float flAddV = (1/m_flTextureScale);

	m_oContext.SetColor(m_clrRope);

	if (!m_bFirstLink)
	{
		// Finish the previous link
		m_oContext.TexCoord(1, m_flTextureOffset+flAddV);
		m_oContext.Vertex(m_vecLastLink+vecLastRight);
		m_oContext.TexCoord(0, m_flTextureOffset+flAddV);
		m_oContext.Vertex(m_vecLastLink-vecLastRight);

		m_flTextureOffset += flAddV;
	}

	m_bFirstLink = false;

	// Start this link
	m_oContext.TexCoord(0, m_flTextureOffset);
	m_oContext.Vertex(m_vecLastLink-vecLastRight);
	m_oContext.TexCoord(1, m_flTextureOffset);
	m_oContext.Vertex(m_vecLastLink+vecLastRight);

	m_vecLastLink = vecLink;
	m_flLastLinkWidth = m_flWidth;
}

void CRopeRenderer::FinishSegment(Vector vecLink, Vector vecNextSegmentStart, float flNextSegmentWidth)
{
	Vector vecForward;
	if (m_bUseForward)
		vecForward = m_vecForward;
	else
		vecForward = m_pRenderer->GetCameraVector();

	Vector vecUp = (vecLink - m_vecLastLink).Normalized();
	Vector vecLastRight = vecForward.Cross(vecUp)*(m_flLastLinkWidth/2);
	Vector vecRight = vecForward.Cross(vecUp)*(m_flWidth/2);

	float flAddV = (1/m_flTextureScale);

	if (m_bFirstLink)
	{
		// Start the previous link
		m_oContext.TexCoord(0, m_flTextureOffset);
		m_oContext.Vertex(m_vecLastLink-vecLastRight);
		m_oContext.TexCoord(1, m_flTextureOffset);
		m_oContext.Vertex(m_vecLastLink+vecLastRight);

		m_flTextureOffset += flAddV;

		m_oContext.TexCoord(1, m_flTextureOffset);
		m_oContext.Vertex(vecLink+vecRight);
		m_oContext.TexCoord(0, m_flTextureOffset);
		m_oContext.Vertex(vecLink-vecRight);
	}
	else
	{
		m_flTextureOffset += flAddV;

		// Finish the last link
		m_oContext.TexCoord(1, m_flTextureOffset);
		m_oContext.Vertex(m_vecLastLink+vecLastRight);
		m_oContext.TexCoord(0, m_flTextureOffset);
		m_oContext.Vertex(m_vecLastLink-vecLastRight);

		m_oContext.TexCoord(0, m_flTextureOffset);
		m_oContext.Vertex(m_vecLastLink-vecLastRight);
		m_oContext.TexCoord(1, m_flTextureOffset);
		m_oContext.Vertex(m_vecLastLink+vecLastRight);

		m_flTextureOffset += flAddV;

		m_oContext.TexCoord(1, m_flTextureOffset);
		m_oContext.Vertex(vecLink+vecRight);
		m_oContext.TexCoord(0, m_flTextureOffset);
		m_oContext.Vertex(vecLink-vecRight);
	}

	m_bFirstLink = true;
	m_vecLastLink = vecNextSegmentStart;
	m_flLastLinkWidth = flNextSegmentWidth;
}

void CRopeRenderer::Finish(Vector vecLink)
{
	Vector vecForward;
	if (m_bUseForward)
		vecForward = m_vecForward;
	else
		vecForward = m_pRenderer->GetCameraVector();

	Vector vecUp = (vecLink - m_vecLastLink).Normalized();
	Vector vecLastRight = vecForward.Cross(vecUp)*(m_flLastLinkWidth/2);
	Vector vecRight = vecForward.Cross(vecUp)*(m_flWidth/2);

	float flAddV = (1/m_flTextureScale);

	if (m_bFirstLink)
	{
		// Start the previous link
		m_oContext.TexCoord(0, m_flTextureOffset);
		m_oContext.Vertex(m_vecLastLink-vecLastRight);
		m_oContext.TexCoord(1, m_flTextureOffset);
		m_oContext.Vertex(m_vecLastLink+vecLastRight);

		m_flTextureOffset += flAddV;

		m_oContext.TexCoord(1, m_flTextureOffset);
		m_oContext.Vertex(vecLink+vecRight);
		m_oContext.TexCoord(0, m_flTextureOffset);
		m_oContext.Vertex(vecLink-vecRight);
	}
	else
	{
		m_flTextureOffset += flAddV;

		// Finish the last link
		m_oContext.TexCoord(1, m_flTextureOffset);
		m_oContext.Vertex(m_vecLastLink+vecLastRight);
		m_oContext.TexCoord(0, m_flTextureOffset);
		m_oContext.Vertex(m_vecLastLink-vecLastRight);

		m_oContext.TexCoord(0, m_flTextureOffset);
		m_oContext.Vertex(m_vecLastLink-vecLastRight);
		m_oContext.TexCoord(1, m_flTextureOffset);
		m_oContext.Vertex(m_vecLastLink+vecLastRight);

		m_flTextureOffset += flAddV;

		m_oContext.TexCoord(1, m_flTextureOffset);
		m_oContext.Vertex(vecLink+vecRight);
		m_oContext.TexCoord(0, m_flTextureOffset);
		m_oContext.Vertex(vecLink-vecRight);
	}

	m_oContext.EndRender();
}

void CRopeRenderer::SetForward(Vector vecForward)
{
	m_bUseForward = true;
	m_vecForward = vecForward;
}
