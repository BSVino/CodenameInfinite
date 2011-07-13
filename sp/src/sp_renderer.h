#ifndef SP_RENDERER_H
#define SP_RENDERER_H

#include <renderer/renderer.h>
#include <common.h>

class CSPRenderer : public CRenderer
{
	DECLARE_CLASS(CSPRenderer, CRenderer);

public:
					CSPRenderer();

public:
	virtual void	PreFrame();
	virtual void	PostFrame();

	virtual void	DrawBackground() {};	// Skybox instead
	virtual void	StartRendering();
	virtual void	RenderSkybox();
	virtual void	RenderGround();

protected:
	size_t			m_iSkyboxFT;
	size_t			m_iSkyboxLF;
	size_t			m_iSkyboxBK;
	size_t			m_iSkyboxRT;
	size_t			m_iSkyboxDN;
	size_t			m_iSkyboxUP;
};

#endif
