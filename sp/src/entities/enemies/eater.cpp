#include "eater.h"

#include <tengine/renderer/game_renderer.h>
#include <tengine/renderer/game_renderingcontext.h>

#include "entities/sp_playercharacter.h"
#include "entities/sp_game.h"
#include "sp_renderer.h"

REGISTER_ENTITY(CEater);

NETVAR_TABLE_BEGIN(CEater);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CEater);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CEater);
INPUTS_TABLE_END();

void CEater::Precache()
{
	PrecacheMaterial("textures/eater.mat");
}

void CEater::Spawn()
{
	m_aabbPhysBoundingBox = AABB(Vector(-0.2f, 0, -0.2f), Vector(0.2f, 0.4f, 0.2f));

	BaseClass::Spawn();

	SetTask(TASK_ATTACK);
}

void CEater::Think()
{
	BaseClass::Think();
}

bool CEater::ShouldRender() const
{
	return SPGame()->GetSPRenderer()->GetRenderingScale() == SCALE_RENDER;
}

void CEater::PostRender() const
{
	BaseClass::PostRender();

	if (GameServer()->GetRenderer()->IsRenderingTransparent())
		return;

	float flRadius = GetPhysBoundingBox().Size().Length()/2;

	CGameRenderingContext c(GameServer()->GetRenderer(), true);

	c.UseMaterial("textures/eater.mat");
	if (GameServer()->GetGameTime() < m_flLastTakeDamage + 0.2)
		c.SetUniform("vecColor", Vector4D(1, 0, 0, 1));

	c.ResetTransformations();
	c.Transform(BaseGetRenderTransform());
	c.Translate(GetLocalUpVector() * (m_aabbPhysBoundingBox.m_vecMaxs-m_aabbPhysBoundingBox.m_vecMins)/2);

	c.RenderBillboard("textures/eater.mat", flRadius);
}

CScalableFloat CEater::CharacterSpeed()
{
	return CScalableFloat(3.0f, SCALE_METER);
}
