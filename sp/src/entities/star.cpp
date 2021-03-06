#include "star.h"

#include <tengine/renderer/game_renderer.h>
#include <tengine/renderer/game_renderingcontext.h>
#include <textures/materiallibrary.h>

#include "sp_game.h"
#include "sp_renderer.h"
#include "sp_playercharacter.h"

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
	PrecacheMaterial("textures/star-yellow-space.mat");
	PrecacheMaterial("textures/star-yellow-atmosphere.mat");
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

	CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
	CPlanet* pNearestPlanet = pCharacter->GetNearestPlanet();

	float flRadius = (float)(GetRadius()*2.0f).GetUnits(SPGame()->GetSPRenderer()->GetRenderingScale());

	CScalableFloat flDistance;
	float flAtmosphere = 0;

	if (pNearestPlanet)
	{
		flDistance = (pNearestPlanet->GetGlobalOrigin() - pCharacter->GetGlobalOrigin()).Length() - pNearestPlanet->GetRadius();
		flAtmosphere = (float)RemapValClamped(flDistance, CScalableFloat(1.0f, SCALE_KILOMETER), pNearestPlanet->GetAtmosphereThickness(), 1.0, 0.0);
	}

	CGameRenderingContext c(GameServer()->GetRenderer(), true);

	c.SetDepthMask(false);

	c.ResetTransformations();
	c.Transform(BaseGetRenderTransform());

	// Set material and uniform now so RenderBillboard doesn't overwrite the alpha value.
	c.UseMaterial("textures/star-yellow-atmosphere.mat");
	c.SetUniform("flAlpha", flAtmosphere);

	c.RenderBillboard("textures/star-yellow-atmosphere.mat", flRadius);

	c.UseMaterial("textures/star-yellow-space.mat");
	c.SetUniform("flAlpha", 1-flAtmosphere);

	c.RenderBillboard("textures/star-yellow-space.mat", flRadius);
}

CScalableFloat CStar::GetCloseOrbit()
{
	// For Earth values this resolves to about 600km above the ground, or about twice the altitude of the ISS.
	return GetRadius()/10.0f;
}
