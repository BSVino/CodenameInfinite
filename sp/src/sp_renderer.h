#ifndef SP_RENDERER_H
#define SP_RENDERER_H

#include <EASTL/vector.h>

#include <common.h>

#include <tengine/game/entities/baseentity.h>
#include <tengine/renderer/game_renderer.h>

#include "sp_common.h"
#include "sp_entity.h"

class CPlanet;
class CStar;

class CSPRenderer : public CGameRenderer
{
	DECLARE_CLASS(CSPRenderer, CGameRenderer);

public:
					CSPRenderer();

public:
	virtual void	PreFrame();

	virtual void	BuildScaleFrustums();
	virtual void	StartRendering(class CRenderingContext* pContext);
	virtual void	DrawSkybox(class CRenderingContext* c);
	virtual void	ModifySkyboxContext(class CRenderingContext* c);
	virtual void	FinishRendering(class CRenderingContext* c);

	void			RenderScale(scale_t eRenderScale, class CRenderingContext* pContext);
	scale_t			GetRenderingScale() { return m_eRenderingScale; }

	bool			IsInFrustumAtScale(scale_t eRenderScale, const Vector& vecCenter, float flRadius);
	bool			IsInFrustumAtScaleSidesOnly(scale_t eRenderScale, const Vector& vecCenter, float flRadius);
	bool			FrustumContainsAtScaleSidesOnly(scale_t eRenderScale, const Vector& vecCenter, float flRadius);
	Vector			ScreenPositionAtScale(scale_t eRenderScale, const Vector& vecWorld);
	Vector			WorldPositionAtScale(scale_t eRenderScale, const Vector& vecScreen);

	class CStar*	GetClosestStar() const;

	float			BloomBrightnessCutoff() const { return 1.25f; }

	virtual bool    ShouldCullByFrustum() const { return false; }

protected:
	CEntityHandle<CStar>	m_hClosestStar;

	// A list of objects to render at which scales.
	tvector<CEntityHandle<CBaseEntity> >	m_ahRenderList;

	scale_t			m_eRenderingScale;
	Frustum			m_aoScaleFrustums[SCALESTACK_SIZE];
	double			m_aiScaleModelViews[SCALESTACK_SIZE][16];
	double			m_aiScaleProjections[SCALESTACK_SIZE][16];
	int				m_aiScaleViewports[SCALESTACK_SIZE][4];
};

#endif
