#ifndef TINKER_RENDERINGCONTEXT_H
#define TINKER_RENDERINGCONTEXT_H

#include <tstring.h>
#include <vector.h>
#include <plane.h>
#include <matrix.h>
#include <color.h>

#include "render_common.h"

class CRenderingContext
{
public:
				CRenderingContext(class CRenderer* pRenderer);
				~CRenderingContext();

public:
	void		Transform(const Matrix4x4& m);
	void		Translate(Vector vecTranslate);
	void		Rotate(float flAngle, Vector vecAxis);
	void		Scale(float flX, float flY, float flZ);
	void		ResetTransformations();

	void		SetBlend(blendtype_t eBlend);
	void		SetAlpha(float flAlpha) { m_flAlpha = flAlpha; };
	void		SetDepthMask(bool bDepthMask);
	void		SetDepthTest(bool bDepthTest);
	void		SetBackCulling(bool bCull);
	void		SetColorSwap(Color clrSwap);
	void		SetLighting(bool bLighting);

	float		GetAlpha() { return m_flAlpha; };
	blendtype_t	GetBlend() { return m_eBlend; };

	void		RenderModel(size_t iModel, class CModel* pCompilingModel = NULL);
	void		RenderSceneNode(class CModel* pModel, class CConversionScene* pScene, class CConversionSceneNode* pNode, class CModel* pCompilingModel);
	void		RenderMeshInstance(class CModel* pModel, class CConversionScene* pScene, class CConversionMeshInstance* pMeshInstance, class CModel* pCompilingModel);

	void		RenderSphere();

	void		RenderBillboard(const tstring& sTexture, float flRadius);

	void		UseFrameBuffer(const class CFrameBuffer* pBuffer);
	void		UseProgram(size_t iProgram);
	void		SetUniform(const char* pszName, int iValue);
	void		SetUniform(const char* pszName, float flValue);
	void		SetUniform(const char* pszName, const Vector& vecValue);
	void		SetUniform(const char* pszName, const Color& vecValue);
	void		BindTexture(const tstring& sName);
	void		BindTexture(size_t iTexture);
	void		SetColor(Color c);
	void		BeginRenderTris();
	void		BeginRenderQuads();
	void		BeginRenderDebugLines();
	void		TexCoord(float s, float t);
	void		TexCoord(const Vector2D& v);
	void		TexCoord(const Vector& v);
	void		Normal(const Vector& v);
	void		Vertex(const Vector& v);
	void		RenderCallList(size_t iCallList);
	void		EndRender();

protected:
	void		PushAttribs();

public:
	CRenderer*	m_pRenderer;

	bool		m_bMatrixTransformations;
	bool		m_bBoundTexture;
	bool		m_bFBO;
	size_t		m_iProgram;
	bool		m_bAttribs;

	bool		m_bColorSwap;
	Color		m_clrSwap;

	blendtype_t	m_eBlend;
	float		m_flAlpha;
};

#endif
