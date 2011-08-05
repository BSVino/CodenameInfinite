#ifndef TINKER_RENDERER_H
#define TINKER_RENDERER_H

#include <EASTL/map.h>
#include <EASTL/vector.h>

#include <tstring.h>
#include <vector.h>
#include <plane.h>
#include <matrix.h>
#include <color.h>

#include "render_common.h"

class CFrameBuffer
{
public:
				CFrameBuffer();

public:
	size_t		m_iWidth;
	size_t		m_iHeight;

	size_t		m_iMap;
	size_t		m_iDepth;
	size_t		m_iFB;

	size_t		m_iCallList;
};

class CRenderBatch
{
public:
	class CModel*	pModel;
	Matrix4x4		mTransformation;
	bool			bSwap;
	Color			clrSwap;
};

#define BLOOM_FILTERS 3

class CRenderer
{
public:
					CRenderer(size_t iWidth, size_t iHeight);

public:
	virtual void	Initialize();

	CFrameBuffer	CreateFrameBuffer(size_t iWidth, size_t iHeight, bool bDepth, bool bLinear);

	void			CreateNoise();
	virtual bool	WantNoise() { return false; };

	virtual void	PreFrame();
	virtual void	PostFrame();

	virtual void	SetupFrame();
	virtual void	DrawBackground();
	virtual void	DrawSkybox();
	virtual void	StartRendering();
	virtual void	FinishRendering();
	virtual void	FinishFrame();
	virtual void	RenderOffscreenBuffers();
	virtual void	RenderFullscreenBuffers();
	virtual void	SetupSceneShader() {};

	void			SetSkybox(size_t ft, size_t bk, size_t lf, size_t rt, size_t up, size_t dn);
	void			DisableSkybox();

	void			RenderBloomPass(CFrameBuffer* apSources, CFrameBuffer* apTargets, bool bHorizontal);

	void			RenderMapFullscreen(size_t iMap);
	void			RenderMapToBuffer(size_t iMap, CFrameBuffer* pBuffer);

	void			SetCameraPosition(Vector vecCameraPosition) { m_vecCameraPosition = vecCameraPosition; };
	void			SetCameraTarget(Vector vecCameraTarget) { m_vecCameraTarget = vecCameraTarget; };
	void			SetCameraUp(Vector vecCameraUp) { m_vecCameraUp = vecCameraUp; };
	void			SetCameraFOV(float flFOV) { m_flCameraFOV = flFOV; };
	void			SetCameraNear(float flNear) { m_flCameraNear = flNear; };
	void			SetCameraFar(float flFar) { m_flCameraFar = flFar; };

	Vector			GetCameraPosition() { return m_vecCameraPosition; };
	Vector			GetCameraTarget() { return m_vecCameraTarget; };
	float			GetCameraFOV() { return m_flCameraFOV; };
	float			GetCameraNear() { return m_flCameraNear; };
	float			GetCameraFar() { return m_flCameraFar; };

	void			FrustumOverride(Vector vecPosition, Vector vecTarget, float flFOV, float flNear, float flFar);
	void			CancelFrustumOverride();

	bool			ShouldBatchThisFrame() { return m_bBatchThisFrame; }
	void			BeginBatching();
	void			AddToBatch(class CModel* pModel, const Matrix4x4& mTransformations, bool bClrSwap, const Color& clrSwap);
	void			RenderBatches();
	bool			IsBatching() { return m_bBatching; };

	Vector			GetCameraVector();
	void			GetCameraVectors(Vector* pvecForward, Vector* pvecRight, Vector* pvecUp);

	bool			IsSphereInFrustum(Vector vecCenter, float flRadius);

	void			SetSize(int w, int h);

	void			ClearProgram();
	void			UseProgram(size_t i);

	Vector			ScreenPosition(Vector vecWorld);
	Vector			WorldPosition(Vector vecScreen);

	const CFrameBuffer*	GetSceneBuffer() { return &m_oSceneBuffer; }

	bool			ShouldUseFramebuffers() { return m_bUseFramebuffers; }
	bool			HardwareSupportsFramebuffers();

	bool			ShouldUseShaders() { return m_bUseShaders; }
	bool			HardwareSupportsShaders();

public:
	static size_t	CreateCallList(size_t iModel);
	static size_t	LoadTextureIntoGL(tstring sFilename, int iClamp = 0);
	static size_t	LoadTextureIntoGL(size_t iImageID, int iClamp = 0);
	static size_t	LoadTextureIntoGL(Color* pclrData, int iClamp = 0);
	static void		UnloadTextureFromGL(size_t iGLID);
	static size_t	GetNumTexturesLoaded() { return s_iTexturesLoaded; }

	static size_t	LoadTextureData(tstring sFilename);
	static Color*	GetTextureData(size_t iTexture);
	static size_t	GetTextureWidth(size_t iTexture);
	static size_t	GetTextureHeight(size_t iTexture);
	static void		UnloadTextureData(size_t iTexture);

protected:
	size_t			m_iWidth;
	size_t			m_iHeight;

	Vector			m_vecCameraPosition;
	Vector			m_vecCameraTarget;
	Vector			m_vecCameraUp;
	float			m_flCameraFOV;
	float			m_flCameraNear;
	float			m_flCameraFar;

	bool			m_bFrustumOverride;
	Vector			m_vecFrustumPosition;
	Vector			m_vecFrustumTarget;
	float			m_flFrustumFOV;
	float			m_flFrustumNear;
	float			m_flFrustumFar;

	double			m_aiModelView[16];
	double			m_aiProjection[16];
	int				m_aiViewport[4];

	Plane			m_aoFrustum[6];

	size_t			m_iSkyboxFT;
	size_t			m_iSkyboxLF;
	size_t			m_iSkyboxBK;
	size_t			m_iSkyboxRT;
	size_t			m_iSkyboxDN;
	size_t			m_iSkyboxUP;

	Vector2D		m_avecSkyboxTexCoords[4];
	Vector			m_avecSkyboxFT[4];
	Vector			m_avecSkyboxBK[4];
	Vector			m_avecSkyboxLF[4];
	Vector			m_avecSkyboxRT[4];
	Vector			m_avecSkyboxUP[4];
	Vector			m_avecSkyboxDN[4];

	bool			m_bBatchThisFrame;
	bool			m_bBatching;
	eastl::map<size_t, eastl::vector<CRenderBatch> > m_aBatches;

	CFrameBuffer	m_oSceneBuffer;

	CFrameBuffer	m_oBloom1Buffers[BLOOM_FILTERS];
	CFrameBuffer	m_oBloom2Buffers[BLOOM_FILTERS];

	CFrameBuffer	m_oNoiseBuffer;

	bool			m_bUseFramebuffers;
	bool			m_bHardwareSupportsFramebuffers;
	bool			m_bHardwareSupportsFramebuffersTestCompleted;

	bool			m_bUseShaders;
	bool			m_bHardwareSupportsShaders;
	bool			m_bHardwareSupportsShadersTestCompleted;

	static size_t	s_iTexturesLoaded;
};

#endif
