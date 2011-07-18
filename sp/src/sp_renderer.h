#ifndef SP_RENDERER_H
#define SP_RENDERER_H

#include <EASTL/vector.h>

#include <common.h>

#include <tengine/game/baseentity.h>
#include <renderer/renderer.h>

#include "sp_common.h"
#include "sp_entity.h"

class CPlanet;
class CStar;

class CSPRenderer : public CRenderer
{
	DECLARE_CLASS(CSPRenderer, CRenderer);

public:
					CSPRenderer();

public:
	virtual void	PreFrame();

	virtual void	DrawBackground() {};	// Skybox instead
	virtual void	StartRendering();
	virtual void	SetupLighting();
	virtual void	RenderSkybox();
	virtual void	FinishRendering();

	void			RenderScale(scale_t eRenderScale);
	scale_t			GetRenderingScale() { return m_eRenderingScale; }

protected:
	size_t			m_iSkyboxFT;
	size_t			m_iSkyboxLF;
	size_t			m_iSkyboxBK;
	size_t			m_iSkyboxRT;
	size_t			m_iSkyboxDN;
	size_t			m_iSkyboxUP;

	CEntityHandle<CStar>	m_hClosestStar;

	// A list of objects to render at which scales.
	eastl::map<size_t, eastl::vector<CEntityHandle<CSPEntity> > >	m_ahRenderScales;

	scale_t			m_eRenderingScale;
};

#endif
