#include "stove.h"

#include <tinker/cvar.h>
#include <tengine/renderer/game_renderer.h>
#include <tengine/renderer/game_renderingcontext.h>

#include "entities/sp_game.h"
#include "sp_renderer.h"
#include "entities/sp_playercharacter.h"
#include "entities/items/pickup.h"
#include "ui/command_menu.h"

#include "powercord.h"

REGISTER_ENTITY(CStove);

NETVAR_TABLE_BEGIN(CStove);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CStove);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, size_t, m_iWoodToBurn);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, double, m_flBurnWoodStart);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CStove);
INPUTS_TABLE_END();

void CStove::Precache()
{
	PrecacheMaterial("textures/stove.mat");
}

void CStove::Spawn()
{
	m_aabbPhysBoundingBox = AABB(Vector(-1.0f, -0.2f, -1.0f), Vector(1.0f, 1.8f, 1.0f));

	BaseClass::Spawn();

	SetTotalHealth(20);
	SetTurnsToConstruct(1);

	m_iWoodToBurn = 0;
	m_flBurnWoodStart = 0;
}

CVar stove_burn_time("stove_burn_time", "6");
CVar stove_power_per_burn("stove_power_per_burn", "20");

void CStove::Think()
{
	BaseClass::Think();

	if (IsUnderConstruction())
		return;

	if (!m_flBurnWoodStart && m_iWoodToBurn)
	{
		m_flBurnWoodStart = GameServer()->GetGameTime();
		m_iWoodToBurn--;
	}

	if (CanAutoOpenMenu())
	{
		// If the player is nearby, looking at me, and not already looking at another command menu, open a temp command menu showing construction info.
		CCommandMenu* pMenu = GameData().CreateCommandMenu(GetOwner()->GetPlayerCharacter());
		SetupMenuButtons();
	}
	else if (CanAutoCloseMenu())
	{
		// If the player goes away or stops looking, close it.
		GameData().CloseCommandMenu();
	}

	if (m_flBurnWoodStart && GameServer()->GetGameTime() > m_flBurnWoodStart + stove_burn_time.GetFloat())
	{
		AddPower(stove_power_per_burn.GetInt());
		m_flBurnWoodStart = 0;
		SetupMenuButtons();
	}
	else if (m_flBurnWoodStart)
	{
		SetupMenuButtons();
	}
}

bool CStove::ShouldRender() const
{
	return SPGame()->GetSPRenderer()->GetRenderingScale() == SCALE_RENDER;
}

void CStove::PostRender() const
{
	BaseClass::PostRender();

	if (IsUnderConstruction() && !GameServer()->GetRenderer()->IsRenderingTransparent())
		return;

	if (!IsUnderConstruction() && GameServer()->GetRenderer()->IsRenderingTransparent())
		return;

	Vector vecUp, vecRight;
	GameServer()->GetRenderer()->GetCameraVectors(nullptr, &vecRight, nullptr);
	vecUp = GetLocalTransform().GetUpVector();
	vecRight *= m_aabbPhysBoundingBox.m_vecMaxs.x;

	Vector vecPosition = BaseGetRenderTransform().GetTranslation();

	if (vecPosition.LengthSqr() > 1000*1000)
		return;

	if (IsOccupied())
		vecPosition += Vector(RandomFloat(-0.01f, 0.01f), RandomFloat(-0.01f, 0.01f), RandomFloat(-0.01f, 0.01f));

	CGameRenderingContext c(GameServer()->GetRenderer(), true);

	c.ResetTransformations();
	c.Translate(vecPosition);

	c.UseMaterial("textures/stove.mat");

	if (GameServer()->GetGameTime() < m_flLastTakeDamage + 0.2)
		c.SetUniform("vecColor", Vector4D(1, 0, 0, 1));

	if (IsUnderConstruction())
	{
		c.SetBlend(BLEND_ALPHA);

		if (GetTotalTurnsToConstruct() == 1)
			c.SetUniform("vecColor", Vector4D(1, 1, 1, 0.5f));
		else
			c.SetUniform("vecColor", Vector4D(1, 1, 1, RemapValClamped((float)GetTurnsToConstruct(), 1, (float)GetTotalTurnsToConstruct(), 0.5f, 0.25f)));
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

void CStove::SetupMenuButtons()
{
	BaseClass::SetupMenuButtons();

	if (IsUnderConstruction())
		return;

	CCommandMenu* pMenu = GameData().GetCommandMenu();

	if (pMenu)
	{
		pMenu->SetTitle(GetStructureName(StructureType()));
		pMenu->SetSubtitle(sprintf("Batteries: %d/%d", GetBatteryLevel(), GetMaxBatteryLevel()));

		if (IsBurning())
			pMenu->SetProgressBar(GameServer()->GetGameTime() - m_flBurnWoodStart, stove_burn_time.GetFloat());
		else
			pMenu->DisableProgressBar();

		pMenu->SetButton(0, "Power", "powercord");
	}
}

void CStove::MenuCommand(const tstring& sCommand)
{
	if (sCommand == "powercord")
	{
		CPowerCord* pCord = GameServer()->Create<CPowerCord>("CPowerCord");
		pCord->SetSource(this);
		GameData().GetCommandMenu()->GetPlayerCharacter()->HoldPowerCord(pCord);
	}
}

void CStove::PerformStructureTask(class CSPCharacter* pCharacter)
{
	BaseClass::PerformStructureTask(pCharacter);

	if (IsUnderConstruction())
		return;
}

bool CStove::IsOccupied() const
{
	if (m_flBurnWoodStart)
		return true;

	return BaseClass::IsOccupied();
}

void CStove::OnPowerDrawn()
{
	if (GameData().GetCommandMenu())
		SetupMenuButtons();
}

size_t CStove::TakeBlocks(item_t eBlock, size_t iNumber)
{
	if (IsUnderConstruction())
		return 0;

	if (eBlock != ITEM_WOOD)
		return 0;

	m_iWoodToBurn += iNumber;

	return iNumber;
}
