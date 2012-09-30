#include "worker.h"

#include <tengine/renderer/game_renderer.h>
#include <tengine/renderer/game_renderingcontext.h>

#include "entities/sp_playercharacter.h"
#include "entities/sp_game.h"
#include "sp_renderer.h"

REGISTER_ENTITY(CWorkerBot);

NETVAR_TABLE_BEGIN(CWorkerBot);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CWorkerBot);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CWorkerBot);
INPUTS_TABLE_END();

void CWorkerBot::Precache()
{
	PrecacheMaterial("textures/workerbot.mat");
}

void CWorkerBot::Spawn()
{
	m_aabbPhysBoundingBox = AABB(Vector(-0.2f, -0.3f, -0.2f), Vector(0.2f, 0.3f, 0.2f));

	BaseClass::Spawn();
}

void CWorkerBot::Think()
{
	BaseClass::Think();
}

bool CWorkerBot::ShouldRender() const
{
	return SPGame()->GetSPRenderer()->GetRenderingScale() == SCALE_RENDER;
}

void CWorkerBot::PostRender() const
{
	BaseClass::PostRender();

	if (GameServer()->GetRenderer()->IsRenderingTransparent())
		return;

	float flRadius = GetPhysBoundingBox().Size().Length()/2;

	CGameRenderingContext c(GameServer()->GetRenderer(), true);

	c.ResetTransformations();
	c.Transform(BaseGetRenderTransform());

	c.RenderBillboard("textures/workerbot.mat", flRadius);
}
