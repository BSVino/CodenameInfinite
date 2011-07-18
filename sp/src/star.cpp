#include "star.h"

#include <tengine/renderer/renderer.h>

#include "sp_game.h"
#include "sp_renderer.h"

REGISTER_ENTITY(CStar);

NETVAR_TABLE_BEGIN(CStar);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CStar);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, float, m_flRadius);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CStar);
INPUTS_TABLE_END();

void CStar::Precache()
{
	PrecacheTexture("textures/star-yellow.png");
}

void CStar::Spawn()
{
	// 695500km, the radius of the sun
	SetRadius(CScalableFloat(695.5f, SCALE_MEGAMETER));
}

void CStar::Think()
{
	BaseClass::Think();
}

float CStar::GetRenderRadius() const
{
	CSPRenderer* pRenderer = SPGame()->GetSPRenderer();
	return m_flRadius.GetUnits(pRenderer->GetRenderingScale());
}

void CStar::PostRender(bool bTransparent) const
{
	BaseClass::PostRender(bTransparent);

	if (!bTransparent)
		return;

	CRenderingContext c(GameServer()->GetRenderer());
	c.Transform(GetGlobalTransform());
	c.SetBlend(BLEND_ADDITIVE);
	c.SetColor(Color(255, 255, 255, 255));
	c.SetLighting(false);

	c.RenderBillboard("textures/star-yellow.png", GetRenderRadius()*2);
}

CScalableFloat CStar::GetCloseOrbit()
{
	// For Earth values this resolves to about 600km above the ground, or about twice the altitude of the ISS.
	return GetRadius()/CScalableFloat(10, m_flRadius.GetScale());
}
