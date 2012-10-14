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

	SetTask(TASK_MINE);
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

	if (IsHoldingABlock())
	{
		CPlayerCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
		Vector vecPosition = GetLocalOrigin() - pCharacter->GetLocalOrigin() + GetLocalUpVector()*0.8f;
		vecPosition -= Vector(0.25f, 0.25f, 0.25f);

		Vector vecForward = BaseGetRenderTransform().GetForwardVector()/2;
		Vector vecUp = BaseGetRenderTransform().GetUpVector()/2;
		Vector vecRight = BaseGetRenderTransform().GetRightVector()/2;

		CGameRenderingContext c(GameServer()->GetRenderer(), true);

		c.ResetTransformations();
		c.Translate(vecPosition);

		c.UseMaterial("textures/items1.mat");

		c.BeginRenderTriFan();
			c.TexCoord(0.0f, 0.75f);
			c.Vertex(Vector(0, 0, 0));
			c.TexCoord(0.25f, 0.75f);
			c.Vertex(vecRight);
			c.TexCoord(0.25f, 1.0f);
			c.Vertex(vecRight + vecUp);
			c.TexCoord(0.0f, 1.0f);
			c.Vertex(vecUp);
			c.TexCoord(0.25f, 1.0f);
			c.Vertex(vecUp + vecForward);
			c.TexCoord(0.25f, 0.75f);
			c.Vertex(vecForward);
			c.TexCoord(0.25f, 1.0f);
			c.Vertex(vecForward + vecRight);
			c.TexCoord(0.0f, 1.0f);
			c.Vertex(vecRight);
		c.EndRender();

		c.BeginRenderTriFan();
			c.TexCoord(0.0f, 1.0f);
			c.Vertex(vecRight + vecUp + vecForward);
			c.TexCoord(0.0f, 0.75f);
			c.Vertex(vecUp + vecForward);
			c.TexCoord(0.25f, 0.75f);
			c.Vertex(vecUp);
			c.TexCoord(0.25f, 1.0f);
			c.Vertex(vecRight + vecUp);
			c.TexCoord(0.25f, 0.75f);
			c.Vertex(vecRight);
			c.TexCoord(0.0f, 0.75f);
			c.Vertex(vecForward + vecRight);
			c.TexCoord(0.25f, 0.75f);
			c.Vertex(vecForward);
			c.TexCoord(0.25f, 1.0f);
			c.Vertex(vecForward + vecUp);
		c.EndRender();
	}
}

CScalableFloat CWorkerBot::CharacterSpeed()
{
	return CScalableFloat(2.0f, SCALE_METER);
}
