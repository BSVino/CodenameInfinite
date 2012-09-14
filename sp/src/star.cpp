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

	CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();

	float flRadius = (float)(GetRadius()*2.0f).GetUnits(SPGame()->GetSPRenderer()->GetRenderingScale());

	if (pCharacter->GetNearestPlanet())
	{
		Vector vecRight = pCharacter->GetGlobalTransform().GetForwardVector().Cross(pCharacter->GetUpVector()).Normalized();
		Vector vecUp = vecRight.Cross(pCharacter->GetGlobalTransform().GetForwardVector()).Normalized();

		CRenderingContext c(GameServer()->GetRenderer(), true);

		// The skybox is drawn globally even if we're in planet-local rendering mode.
		c.SetView(Matrix4x4::ConstructCameraView(Vector(0, 0, 0), AngleVector(pCharacter->GetGlobalAngles()), pCharacter->GetUpVector()));

		c.ResetTransformations();
		c.Transform(GetGlobalTransform().GetUnits(SPGame()->GetSPRenderer()->GetRenderingScale()));

		c.RenderBillboard(CMaterialHandle("textures/star-yellow.mat"), flRadius, vecUp, vecRight);
	}
	else
	{
		CGameRenderingContext c(GameServer()->GetRenderer(), true);

		c.ResetTransformations();
		c.Transform(BaseGetRenderTransform());

		c.RenderBillboard("textures/star-yellow.mat", flRadius);
	}
}

CScalableFloat CStar::GetCloseOrbit()
{
	// For Earth values this resolves to about 600km above the ground, or about twice the altitude of the ISS.
	return GetRadius()/10.0f;
}
