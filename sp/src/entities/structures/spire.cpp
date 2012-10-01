#include "spire.h"

#include <tengine/renderer/game_renderer.h>
#include <tengine/renderer/game_renderingcontext.h>

#include "entities/sp_game.h"
#include "sp_renderer.h"
#include "entities/sp_playercharacter.h"
#include "game/gameserver.h"
#include "entities/bots/worker.h"

REGISTER_ENTITY(CSpire);

NETVAR_TABLE_BEGIN(CSpire);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CSpire);
	SAVEDATA_DEFINE(CSaveData::DATA_STRING, tstring, m_sBaseName);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CSpire);
INPUTS_TABLE_END();

void CSpire::Precache()
{
	PrecacheMaterial("textures/spire.mat");
}

void CSpire::Spawn()
{
	m_aabbPhysBoundingBox = AABB(Vector(-1.0f, -1.0f, -1.0f), Vector(1.0f, 15.0f, 1.0f));

	BaseClass::Spawn();
}

bool CSpire::ShouldRender() const
{
	return SPGame()->GetSPRenderer()->GetRenderingScale() == SCALE_RENDER;
}

void CSpire::PostRender() const
{
	BaseClass::PostRender();

	if (GameServer()->GetRenderer()->IsRenderingTransparent())
		return;

	Vector vecUp, vecRight;
	GameServer()->GetRenderer()->GetCameraVectors(nullptr, &vecRight, nullptr);
	vecUp = GetLocalTransform().GetUpVector();
	vecRight *= m_aabbPhysBoundingBox.m_vecMaxs.x;

	Vector vecPosition = BaseGetRenderTransform().GetTranslation();

	if (vecPosition.LengthSqr() > 1000*1000)
		return;

	CGameRenderingContext c(GameServer()->GetRenderer(), true);

	c.ResetTransformations();
	c.Translate(vecPosition);

	c.UseMaterial("textures/spire.mat");

	c.BeginRenderTriFan();
		c.TexCoord(0.0f, 1.0f);
		c.Vertex(-vecRight + vecUp * m_aabbPhysBoundingBox.m_vecMaxs.y);
		c.TexCoord(0.0f, 0.0f);
		c.Vertex(-vecRight + vecUp * m_aabbPhysBoundingBox.m_vecMins.y);
		c.TexCoord(1.0f, 0.0f);
		c.Vertex(vecRight + vecUp * m_aabbPhysBoundingBox.m_vecMins.y);
		c.TexCoord(1.0f, 1.0f);
		c.Vertex(vecRight + vecUp * m_aabbPhysBoundingBox.m_vecMaxs.y);
	c.EndRender();
}

void CSpire::PostConstruction()
{
	BaseClass::PostConstruction();

	CWorkerBot* pWorker = GameServer()->Create<CWorkerBot>("CWorkerBot");
	pWorker->SetOwner(GetOwner());
	pWorker->SetMoveParent(GetMoveParent());
	pWorker->SetLocalOrigin(GetLocalOrigin() + GetLocalTransform().GetUpVector() + GetLocalTransform().GetRightVector()*2);
}