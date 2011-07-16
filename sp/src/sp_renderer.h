#ifndef SP_RENDERER_H
#define SP_RENDERER_H

#include <EASTL/vector.h>

#include <common.h>

#include <tengine/game/baseentity.h>
#include <renderer/renderer.h>

class CPlanet;

class CSPRenderer : public CRenderer
{
	DECLARE_CLASS(CSPRenderer, CRenderer);

public:
					CSPRenderer();

public:
	virtual void	PreFrame();

	virtual void	DrawBackground() {};	// Skybox instead
	virtual void	StartRendering();
	virtual void	RenderSkybox();

	void			AddPlanetToUpdate(CPlanet* pPlanet);

protected:
	size_t			m_iSkyboxFT;
	size_t			m_iSkyboxLF;
	size_t			m_iSkyboxBK;
	size_t			m_iSkyboxRT;
	size_t			m_iSkyboxDN;
	size_t			m_iSkyboxUP;

	eastl::vector<CEntityHandle<CPlanet> >	m_ahPlanetsToUpdate;
};

#endif
