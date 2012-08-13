#include "star.h"

#include <tengine/renderer/game_renderer.h>
#include <tengine/renderer/game_renderingcontext.h>
#include <tinker/textures/materialhandle.h>

#include "sp_game.h"
#include "sp_renderer.h"

REGISTER_ENTITY(CStar);

NETVAR_TABLE_BEGIN(CStar);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CStar);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, float, m_flRadius);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, Color, m_clrLight);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CStar);
INPUTS_TABLE_END();

void CStar::Precache()
{
	PrecacheMaterial("textures/star-yellow.mat");
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

void CStar::PostRender() const
{
	BaseClass::PostRender();

	if (GameServer()->GetRenderer()->IsRenderingTransparent())
		return;

	CGameRenderingContext c(GameServer()->GetRenderer(), true);

	c.ResetTransformations();
	c.Transform(GetRenderTransform());

	c.RenderBillboard("textures/star-yellow.mat", (float)(GetRadius()*2.0f).GetUnits(SPGame()->GetSPRenderer()->GetRenderingScale()));
}

CScalableFloat CStar::GetCloseOrbit()
{
	// For Earth values this resolves to about 600km above the ground, or about twice the altitude of the ISS.
	return GetRadius()/10.0f;
}
