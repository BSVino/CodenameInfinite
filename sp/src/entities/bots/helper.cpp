#include "helper.h"

#include <tengine/renderer/game_renderer.h>
#include <tengine/renderer/game_renderingcontext.h>

#include "entities/sp_playercharacter.h"
#include "entities/sp_game.h"
#include "sp_renderer.h"

REGISTER_ENTITY(CHelperBot);

NETVAR_TABLE_BEGIN(CHelperBot);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CHelperBot);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CPlayerCharacter, m_hPlayer);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CHelperBot);
INPUTS_TABLE_END();

void CHelperBot::Precache()
{
	PrecacheMaterial("textures/helperbot.mat");
}

void CHelperBot::Spawn()
{
	m_aabbPhysBoundingBox = AABB(Vector(-0.2f, -0.3f, -0.2f), Vector(0.2f, 0.3f, 0.2f));

	BaseClass::Spawn();
}

void CHelperBot::Think()
{
	if (m_hPlayer)
	{
		if ((GetGlobalOrigin() - m_hPlayer->GetGlobalOrigin()).LengthSqr() > CScalableFloat(25))
			TeleportToPlayer();
	}

	BaseClass::Think();
}

void CHelperBot::TeleportToPlayer()
{
	if (!m_hPlayer)
		return;

	SetGlobalOrigin(m_hPlayer->GetGlobalOrigin() + m_hPlayer->GetUpVector()*1.5 + m_hPlayer->GetGlobalTransform().GetRightVector());
}

void CHelperBot::SetPlayerCharacter(CPlayerCharacter* pPlayer)
{
	m_hPlayer = pPlayer;

	if (!pPlayer)
	{
		Delete();
		return;
	}

	TeleportToPlayer();
}

bool CHelperBot::ShouldRender() const
{
	return SPGame()->GetSPRenderer()->GetRenderingScale() == SCALE_RENDER;
}

void CHelperBot::PostRender() const
{
	BaseClass::PostRender();

	if (GameServer()->GetRenderer()->IsRenderingTransparent())
		return;

	float flRadius = GetPhysBoundingBox().Size().Length()/2;

	CGameRenderingContext c(GameServer()->GetRenderer(), true);

	c.ResetTransformations();
	c.Transform(BaseGetRenderTransform());

	c.RenderBillboard("textures/helperbot.mat", flRadius);
}
