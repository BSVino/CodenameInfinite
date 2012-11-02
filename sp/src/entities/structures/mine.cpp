#include "mine.h"

#include <tinker/cvar.h>
#include <tengine/renderer/game_renderer.h>
#include <tengine/renderer/game_renderingcontext.h>

#include "entities/sp_game.h"
#include "sp_renderer.h"
#include "entities/sp_playercharacter.h"
#include "entities/items/pickup.h"

REGISTER_ENTITY(CMine);

NETVAR_TABLE_BEGIN(CMine);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CMine);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CMine);
INPUTS_TABLE_END();

void CMine::Precache()
{
	PrecacheMaterial("textures/mine.mat");
}

void CMine::Spawn()
{
	m_aabbPhysBoundingBox = AABB(Vector(-1.0f, -0.2f, -1.0f), Vector(1.0f, 1.8f, 1.0f));

	SetTurnsToConstruct(3);

	BaseClass::Spawn();

	m_flDiggingStarted = 0;
}

CVar mine_digtime("mine_digtime", "3");

void CMine::Think()
{
	BaseClass::Think();

	if (m_flDiggingStarted && m_flDiggingStarted + mine_digtime.GetFloat() < GameServer()->GetGameTime())
	{
		m_flDiggingStarted = 0;

		CPickup* pMined = GameServer()->Create<CPickup>("CPickup");
		pMined->GameData().SetPlayerOwner(GameData().GetPlayerOwner());
		pMined->GameData().SetPlanet(GameData().GetPlanet());
		pMined->SetGlobalTransform(GameData().GetPlanet()->GetGlobalTransform()); // Avoid floating point precision problems
		pMined->SetMoveParent(GameData().GetPlanet());
		pMined->SetLocalTransform(GetLocalTransform());
		pMined->SetLocalOrigin(GetLocalOrigin() + GetLocalTransform().GetUpVector() + GetLocalTransform().GetRightVector());
		pMined->SetItem(ITEM_DIRT);
	}
}

bool CMine::ShouldRender() const
{
	return SPGame()->GetSPRenderer()->GetRenderingScale() == SCALE_RENDER;
}

void CMine::PostRender() const
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

	if (m_flDiggingStarted)
		vecPosition += Vector(RandomFloat(-0.01f, 0.01f), RandomFloat(-0.01f, 0.01f), RandomFloat(-0.01f, 0.01f));

	if (IsWorkingConstructionTurn())
		vecPosition += Vector(RandomFloat(-0.01f, 0.01f), RandomFloat(-0.01f, 0.01f), RandomFloat(-0.01f, 0.01f));

	CGameRenderingContext c(GameServer()->GetRenderer(), true);

	c.ResetTransformations();
	c.Translate(vecPosition);

	c.UseMaterial("textures/mine.mat");

	if (GameServer()->GetGameTime() < m_flLastTakeDamage + 0.2)
		c.SetUniform("vecColor", Vector4D(1, 0, 0, 1));

	if (IsUnderConstruction())
	{
		c.SetBlend(BLEND_ALPHA);

		if (GetTotalTurnsToConstruct() == 1)
			c.SetUniform("vecColor", Vector4D(1, 1, 1, 0.5f));
		else
			c.SetUniform("vecColor", Vector4D(1, 1, 1, RemapValClamped((float)GetTurnsToConstruct(), 1, (float)GetTotalTurnsToConstruct(), 0.65f, 0.25f)));
	}

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

void CMine::PerformStructureTask(CSPCharacter* pCharacter)
{
	BaseClass::PerformStructureTask(pCharacter);

	if (IsUnderConstruction())
		return;

	if (m_flDiggingStarted)
		return;

	m_flDiggingStarted = GameServer()->GetGameTime();
}
