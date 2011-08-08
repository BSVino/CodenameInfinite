#include "renderingcontext.h"

#include <GL/glew.h>
#include <IL/il.h>
#include <IL/ilu.h>

#include <maths.h>
#include <simplex.h>

#include <modelconverter/convmesh.h>
#include <models/models.h>
#include <shaders/shaders.h>
#include <tinker/application.h>
#include <tinker/cvar.h>
#include <tinker/profiler.h>
#include <game/gameserver.h>
#include <models/texturelibrary.h>
#include <renderer/renderer.h>

CRenderingContext::CRenderingContext(CRenderer* pRenderer)
{
	m_pRenderer = pRenderer;

	m_bMatrixTransformations = false;
	m_bBoundTexture = false;
	m_bFBO = false;
	m_iProgram = 0;
	m_bAttribs = false;

	m_bColorSwap = false;

	m_eBlend = BLEND_NONE;
	m_flAlpha = 1;
}

CRenderingContext::~CRenderingContext()
{
	if (m_bMatrixTransformations)
		glPopMatrix();

	if (m_bBoundTexture)
		glBindTexture(GL_TEXTURE_2D, 0);

	if (m_bFBO)
	{
		glBindFramebufferEXT(GL_FRAMEBUFFER, (GLuint)m_pRenderer->GetSceneBuffer()->m_iFB);
		glViewport(0, 0, (GLsizei)m_pRenderer->GetSceneBuffer()->m_iWidth, (GLsizei)m_pRenderer->GetSceneBuffer()->m_iHeight);
	}

	if (m_iProgram)
		glUseProgram(0);

	if (m_bAttribs)
		glPopAttrib();
}

void CRenderingContext::Transform(const Matrix4x4& m)
{
	if (!m_bMatrixTransformations)
	{
		m_bMatrixTransformations = true;
		glPushMatrix();
	}

	glMultMatrixf(m.Transposed());	// GL uses column major.
}

void CRenderingContext::Translate(Vector vecTranslate)
{
	if (!m_bMatrixTransformations)
	{
		m_bMatrixTransformations = true;
		glPushMatrix();
	}

	glTranslatef(vecTranslate.x, vecTranslate.y, vecTranslate.z);
}

void CRenderingContext::Rotate(float flAngle, Vector vecAxis)
{
	if (!m_bMatrixTransformations)
	{
		m_bMatrixTransformations = true;
		glPushMatrix();
	}

	glRotatef(flAngle, vecAxis.x, vecAxis.y, vecAxis.z);
}

void CRenderingContext::Scale(float flX, float flY, float flZ)
{
	if (!m_bMatrixTransformations)
	{
		m_bMatrixTransformations = true;
		glPushMatrix();
	}

	glScalef(flX, flY, flZ);
}

void CRenderingContext::ResetTransformations()
{
	if (m_bMatrixTransformations)
	{
		m_bMatrixTransformations = false;
		glPopMatrix();
	}
}

void CRenderingContext::SetBlend(blendtype_t eBlend)
{
	if (!m_bAttribs)
		PushAttribs();

	if (eBlend)
	{
		glEnable(GL_BLEND);

		if (eBlend == BLEND_ALPHA)
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		else
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	}
	else
	{
		glDisable(GL_BLEND);
	}

	m_eBlend = eBlend;
}

void CRenderingContext::SetDepthMask(bool bDepthMask)
{
	if (!m_bAttribs)
		PushAttribs();

	glDepthMask(bDepthMask);
}

void CRenderingContext::SetDepthTest(bool bDepthTest)
{
	if (!m_bAttribs)
		PushAttribs();

	if (bDepthTest)
		glEnable(GL_DEPTH_TEST);
	else
		glDisable(GL_DEPTH_TEST);
}

void CRenderingContext::SetBackCulling(bool bCull)
{
	if (!m_bAttribs)
		PushAttribs();

	if (bCull)
		glEnable(GL_CULL_FACE);
	else
		glDisable(GL_CULL_FACE);
}

void CRenderingContext::SetColorSwap(Color clrSwap)
{
	m_bColorSwap = true;
	m_clrSwap = clrSwap;
}

void CRenderingContext::SetLighting(bool bLighting)
{
	if (!m_bAttribs)
		PushAttribs();

	if (bLighting)
		glEnable(GL_LIGHTING);
	else
		glDisable(GL_LIGHTING);
}

void CRenderingContext::RenderModel(size_t iModel, CModel* pCompilingModel)
{
	CModel* pModel = CModelLibrary::Get()->GetModel(iModel);

	if (!pModel)
		return;

	if (pModel->m_bStatic && !pCompilingModel)
	{
		if (m_pRenderer->IsBatching())
		{
			TAssert(m_eBlend == BLEND_NONE);

			Matrix4x4 mTransformations;
			glGetFloatv(GL_MODELVIEW_MATRIX, mTransformations);

			m_pRenderer->AddToBatch(pModel, mTransformations, m_bColorSwap, m_clrSwap);
		}
		else
		{
			glPushAttrib(GL_ENABLE_BIT|GL_CURRENT_BIT|GL_LIGHTING_BIT|GL_TEXTURE_BIT);

			TAssert(pModel->m_iCallListTexture);
			glBindTexture(GL_TEXTURE_2D, pModel->m_iCallListTexture);

			if (m_pRenderer->ShouldUseShaders())
			{
				GLuint iProgram = (GLuint)CShaderLibrary::GetModelProgram();
				glUseProgram(iProgram);

				GLuint bDiffuse = glGetUniformLocation(iProgram, "bDiffuse");
				glUniform1i(bDiffuse, true);

				GLuint iDiffuse = glGetUniformLocation(iProgram, "iDiffuse");
				glUniform1i(iDiffuse, 0);

				GLuint flAlpha = glGetUniformLocation(iProgram, "flAlpha");
				glUniform1f(flAlpha, m_flAlpha);

				GLuint bColorSwapInAlpha = glGetUniformLocation(iProgram, "bColorSwapInAlpha");
				glUniform1i(bColorSwapInAlpha, m_bColorSwap);

				if (m_bColorSwap)
				{
					GLuint vecColorSwap = glGetUniformLocation(iProgram, "vecColorSwap");
					Vector vecColor((float)m_clrSwap.r()/255, (float)m_clrSwap.g()/255, (float)m_clrSwap.b()/255);
					glUniform3fv(vecColorSwap, 1, vecColor);
				}

				glColor4f(1, 1, 1, 1);

				glCallList((GLuint)pModel->m_iCallList);

				glUseProgram(0);
			}
			else
			{
				if (m_bColorSwap)
					glColor4f(((float)m_clrSwap.r())/255, ((float)m_clrSwap.g())/255, ((float)m_clrSwap.b())/255, m_flAlpha);
				else
					glColor4f(1, 1, 1, m_flAlpha);

				glCallList((GLuint)pModel->m_iCallList);
			}

			glPopAttrib();
		}
	}
	else
	{
		for (size_t i = 0; i < pModel->m_pScene->GetNumScenes(); i++)
			RenderSceneNode(pModel, pModel->m_pScene, pModel->m_pScene->GetScene(i), pCompilingModel);
	}

	if (!pCompilingModel && m_pRenderer->ShouldUseShaders() && !m_pRenderer->IsBatching())
		m_pRenderer->ClearProgram();
}

void CRenderingContext::RenderSceneNode(CModel* pModel, CConversionScene* pScene, CConversionSceneNode* pNode, CModel* pCompilingModel)
{
	if (!pNode)
		return;

	if (!pNode->IsVisible())
		return;

	bool bTransformationsIdentity = false;
	if (pNode->m_mTransformations.IsIdentity())
		bTransformationsIdentity = true;

	if (!bTransformationsIdentity)
	{
		glPushMatrix();

		glMultMatrixf(pNode->m_mTransformations.Transposed());	// GL uses column major.
	}

	for (size_t i = 0; i < pNode->GetNumChildren(); i++)
		RenderSceneNode(pModel, pScene, pNode->GetChild(i), pCompilingModel);

	for (size_t m = 0; m < pNode->GetNumMeshInstances(); m++)
		RenderMeshInstance(pModel, pScene, pNode->GetMeshInstance(m), pCompilingModel);

	if (!bTransformationsIdentity)
		glPopMatrix();
}

void CRenderingContext::RenderMeshInstance(CModel* pModel, CConversionScene* pScene, CConversionMeshInstance* pMeshInstance, CModel* pCompilingModel)
{
	if (!pMeshInstance->IsVisible())
		return;

	if (!pCompilingModel)
		glPushAttrib(GL_ENABLE_BIT|GL_CURRENT_BIT|GL_LIGHTING_BIT|GL_TEXTURE_BIT);

	CConversionMesh* pMesh = pMeshInstance->GetMesh();

	for (size_t j = 0; j < pMesh->GetNumFaces(); j++)
	{
		size_t k;
		CConversionFace* pFace = pMesh->GetFace(j);

		if (pFace->m == ~0)
			continue;

		CConversionMaterial* pMaterial = NULL;
		CConversionMaterialMap* pConversionMaterialMap = pMeshInstance->GetMappedMaterial(pFace->m);

		if (pConversionMaterialMap)
		{
			if (!pConversionMaterialMap->IsVisible())
				continue;

			pMaterial = pScene->GetMaterial(pConversionMaterialMap->m_iMaterial);
			if (pMaterial && !pMaterial->IsVisible())
				continue;
		}

		if (pCompilingModel)
		{
			if (pMaterial)
			{
				GLuint iTexture = (GLuint)pModel->m_aiTextures[pConversionMaterialMap->m_iMaterial];

				if (!pModel->m_iCallListTexture)
					pModel->m_iCallListTexture = iTexture;
				else
					// If you hit this you have more than one texture in a call list that's building.
					// That's a no-no because these call lists are batched.
					TAssert(pModel->m_iCallListTexture == iTexture);
			}
		}

		if (!pCompilingModel)
		{
			bool bTexture = false;
			if (pMaterial)
			{
				GLuint iTexture = (GLuint)pModel->m_aiTextures[pConversionMaterialMap->m_iMaterial];
				glBindTexture(GL_TEXTURE_2D, iTexture);

				bTexture = !!iTexture;
			}
			else
				glBindTexture(GL_TEXTURE_2D, 0);

			if (m_pRenderer->ShouldUseShaders())
			{
				GLuint iProgram = (GLuint)CShaderLibrary::GetModelProgram();
				glUseProgram(iProgram);

				GLuint bDiffuse = glGetUniformLocation(iProgram, "bDiffuse");
				glUniform1i(bDiffuse, bTexture);

				GLuint iDiffuse = glGetUniformLocation(iProgram, "iDiffuse");
				glUniform1i(iDiffuse, 0);

				GLuint flAlpha = glGetUniformLocation(iProgram, "flAlpha");
				glUniform1f(flAlpha, m_flAlpha);

				GLuint bColorSwapInAlpha = glGetUniformLocation(iProgram, "bColorSwapInAlpha");
				glUniform1i(bColorSwapInAlpha, m_bColorSwap);

				if (m_bColorSwap)
				{
					GLuint vecColorSwap = glGetUniformLocation(iProgram, "vecColorSwap");
					Vector vecColor((float)m_clrSwap.r()/255, (float)m_clrSwap.g()/255, (float)m_clrSwap.b()/255);
					glUniform3fv(vecColorSwap, 1, vecColor);
				}
			}
			else
			{
				if (m_bColorSwap)
					glColor4f(((float)m_clrSwap.r())/255, ((float)m_clrSwap.g())/255, ((float)m_clrSwap.b())/255, m_flAlpha);
				else
					glColor4f(pMaterial->m_vecDiffuse.x, pMaterial->m_vecDiffuse.y, pMaterial->m_vecDiffuse.z, m_flAlpha);
			}
		}

		glBegin(GL_POLYGON);

		for (k = 0; k < pFace->GetNumVertices(); k++)
		{
			CConversionVertex* pVertex = pFace->GetVertex(k);

			glTexCoord2fv(pMesh->GetUV(pVertex->vu));
			glNormal3fv(pMesh->GetNormal(pVertex->vn));
			glVertex3fv(pMesh->GetVertex(pVertex->v));
		}

		glEnd();
	}

	if (!pCompilingModel)
		glPopAttrib();
}

void CRenderingContext::RenderSphere()
{
	static size_t iSphereCallList = 0;

	if (iSphereCallList == 0)
	{
		GLUquadricObj* pQuadric = gluNewQuadric();
		iSphereCallList = glGenLists(1);
		glNewList((GLuint)iSphereCallList, GL_COMPILE);
		gluSphere(pQuadric, 1, 20, 10);
		glEndList();
		gluDeleteQuadric(pQuadric);
	}

	glCallList(iSphereCallList);
}

void CRenderingContext::RenderBillboard(const tstring& sTexture, float flRadius)
{
	size_t iTexture = CTextureLibrary::FindTextureID(sTexture);

	Vector vecUp, vecRight;
	m_pRenderer->GetCameraVectors(NULL, &vecRight, &vecUp);

	vecUp *= flRadius;
	vecRight *= flRadius;

	BindTexture(iTexture);
	BeginRenderQuads();
		TexCoord(0, 1);
		Vertex(-vecRight + vecUp);
		TexCoord(0, 0);
		Vertex(-vecRight - vecUp);
		TexCoord(1, 0);
		Vertex(vecRight - vecUp);
		TexCoord(1, 1);
		Vertex(vecRight + vecUp);
	EndRender();
}

void CRenderingContext::UseFrameBuffer(const CFrameBuffer* pBuffer)
{
	TAssert(m_pRenderer->ShouldUseFramebuffers());

	m_bFBO = true;
	glBindFramebufferEXT(GL_FRAMEBUFFER, (GLuint)pBuffer->m_iFB);
	glViewport(0, 0, (GLsizei)pBuffer->m_iWidth, (GLsizei)pBuffer->m_iHeight);
}

void CRenderingContext::UseProgram(size_t iProgram)
{
	TAssert(m_pRenderer->ShouldUseShaders());

	if (!m_pRenderer->ShouldUseShaders())
		return;

	m_iProgram = iProgram;
	glUseProgram((GLuint)iProgram);
}

void CRenderingContext::SetUniform(const char* pszName, int iValue)
{
	TAssert(m_pRenderer->ShouldUseShaders());

	if (!m_pRenderer->ShouldUseShaders())
		return;

	int iUniform = glGetUniformLocation((GLuint)m_iProgram, pszName);
	glUniform1i(iUniform, iValue);
}

void CRenderingContext::SetUniform(const char* pszName, float flValue)
{
	TAssert(m_pRenderer->ShouldUseShaders());

	if (!m_pRenderer->ShouldUseShaders())
		return;

	int iUniform = glGetUniformLocation((GLuint)m_iProgram, pszName);
	glUniform1f(iUniform, flValue);
}

void CRenderingContext::SetUniform(const char* pszName, const Vector& vecValue)
{
	TAssert(m_pRenderer->ShouldUseShaders());

	if (!m_pRenderer->ShouldUseShaders())
		return;

	int iUniform = glGetUniformLocation((GLuint)m_iProgram, pszName);
	glUniform3fv(iUniform, 1, vecValue);
}

void CRenderingContext::SetUniform(const char* pszName, const Color& clrValue)
{
	TAssert(m_pRenderer->ShouldUseShaders());

	if (!m_pRenderer->ShouldUseShaders())
		return;

	int iUniform = glGetUniformLocation((GLuint)m_iProgram, pszName);
	glUniform3fv(iUniform, 1, Vector(clrValue));
}

void CRenderingContext::BindTexture(const tstring& sName)
{
	BindTexture(CTextureLibrary::GetTextureGLID(sName));
}

void CRenderingContext::BindTexture(size_t iTexture)
{
	if (!m_bAttribs)
		PushAttribs();

	glBindTexture(GL_TEXTURE_2D, (GLuint)iTexture);
	m_bBoundTexture = true;
}

void CRenderingContext::SetColor(Color c)
{
	if (!m_bAttribs)
		PushAttribs();

	glColor4ub(c.r(), c.g(), c.b(), (unsigned char)(c.a()*m_flAlpha));
}

void CRenderingContext::BeginRenderTris()
{
	glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);

	m_avecTexCoords.clear();
	m_avecNormals.clear();
	m_avecVertices.clear();

	m_bTexCoord = false;
	m_bNormal = false;

	m_iDrawMode = GL_TRIANGLES;
}

void CRenderingContext::BeginRenderQuads()
{
	glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);

	m_avecTexCoords.clear();
	m_avecNormals.clear();
	m_avecVertices.clear();

	m_bTexCoord = false;
	m_bNormal = false;

	m_iDrawMode = GL_QUADS;
}

void CRenderingContext::BeginRenderDebugLines()
{
	glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);

	m_avecTexCoords.clear();
	m_avecNormals.clear();
	m_avecVertices.clear();

	m_bTexCoord = false;
	m_bNormal = false;

	glLineWidth( 3.0f );
	m_iDrawMode = GL_LINE_LOOP;
}

void CRenderingContext::TexCoord(float s, float t)
{
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	m_vecTexCoord = Vector2D(s, t);
	m_bTexCoord = true;
}

void CRenderingContext::TexCoord(const Vector2D& v)
{
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	m_vecTexCoord = v;
	m_bTexCoord = true;
}

void CRenderingContext::TexCoord(const DoubleVector2D& v)
{
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	m_vecTexCoord = Vector2D(v);
	m_bTexCoord = true;
}

void CRenderingContext::TexCoord(const Vector& v)
{
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	m_vecTexCoord = v;
	m_bTexCoord = true;
}

void CRenderingContext::Normal(const Vector& v)
{
	glEnableClientState(GL_NORMAL_ARRAY);
	m_vecNormal = v;
	m_bNormal = true;
}

void CRenderingContext::Vertex(const Vector& v)
{
	glEnableClientState(GL_VERTEX_ARRAY);

	if (m_bTexCoord)
		m_avecTexCoords.push_back(m_vecTexCoord);

	if (m_bNormal)
		m_avecNormals.push_back(m_vecNormal);

	m_avecVertices.push_back(v);
}

void CRenderingContext::RenderCallList(size_t iCallList)
{
	glBegin(GL_TRIANGLES);
	glCallList((GLuint)iCallList);
	glEnd();
}

void CRenderingContext::EndRender()
{
	glClientActiveTexture(GL_TEXTURE0);
	if (m_bTexCoord)
		glTexCoordPointer(2, GL_FLOAT, 0, m_avecTexCoords.data());
	if (m_bNormal)
		glNormalPointer(GL_FLOAT, 0, m_avecNormals.data());
	glVertexPointer(3, GL_FLOAT, 0, m_avecVertices.data());
	glDrawArrays(m_iDrawMode, 0, m_avecVertices.size());

	glPopClientAttrib();
}

void CRenderingContext::PushAttribs()
{
	m_bAttribs = true;
	// Push all the attribs we'll ever need. I don't want to have to worry about popping them in order.
	glPushAttrib(GL_ENABLE_BIT|GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_CURRENT_BIT|GL_TEXTURE_BIT);
}

