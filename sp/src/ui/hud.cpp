#include "hud.h"

#include <tengine/game/gameserver.h>
#include <tengine/renderer/game_renderer.h>
#include <glgui/rootpanel.h>
#include <glgui/label.h>
#include <tinker/cvar.h>

#include "planet/planet.h"
#include "entities/sp_playercharacter.h"
#include "entities/sp_game.h"
#include "entities/bots/helper.h"
#include "entities/structures/spire.h"

#include "hudmenu.h"

CSPHUD::CSPHUD()
	: CHUDViewport()
{
}

void CSPHUD::BuildMenus()
{
	for (int i = 0; i < MENU_TOTAL; i++)
	{
		glgui::RootPanel()->RemoveControl(m_ahMenus[i]);

		if (m_ahMenus[i])
			m_ahMenus[i]->ClearSubmenus();
	}

	if (!SPGame())
		return;

	if (SPGame()->GetNumLocalPlayers() == 0)
		return;

	CSPPlayer* pPlayer = SPGame()->GetLocalSPPlayer();

	if (!pPlayer)
		return;

	CPlayerCharacter* pCharacter = pPlayer->GetPlayerCharacter();
	if (!pCharacter)
		return;

	if (pCharacter->HasBlocks())
	{
		m_ahMenus[MENU_BLOCKS] = glgui::RootPanel()->AddControl(new CHUDMenu(MENU_BLOCKS, sprintf("%d. Blocks", MENU_BLOCKS+1)));

		for (size_t i = 1; i < ITEM_BLOCKS_TOTAL; i++)
		{
			if (pCharacter->GetInventory((item_t)i))
				m_ahMenus[MENU_BLOCKS]->AddSubmenu(new CHUDMenu(i-1, sprintf("%d. " + tstring(GetItemName((item_t)i)) + " (x%d)", i, pCharacter->GetInventory((item_t)i)), true), this, PlaceBlock);
		}
	}

	if (pCharacter->GetNearestSpire() || pPlayer->GetNumSpires())
	{
		m_ahMenus[MENU_CONSTRUCTION] = glgui::RootPanel()->AddControl(new CHUDMenu(MENU_CONSTRUCTION, sprintf("%d. Construction", MENU_CONSTRUCTION+1)));

		if (pPlayer->GetNumSpires())
			m_ahMenus[MENU_CONSTRUCTION]->AddSubmenu(new CHUDMenu(0, "1. Spire", true), this, ConstructSpire);

		if (pCharacter->GetNearestSpire())
		{
			m_ahMenus[MENU_CONSTRUCTION]->AddSubmenu(new CHUDMenu(1, "2. Mine", true), this, ConstructMine);
			m_ahMenus[MENU_CONSTRUCTION]->AddSubmenu(new CHUDMenu(2, "3. Pallet", true), this, ConstructPallet);
		}
	}

	glgui::RootPanel()->Layout();
}

tstring GetStringDistance(const CScalableFloat& v)
{
	float flDistance = (float)v.GetUnits(SCALE_KILOMETER);
	if (flDistance > 10000000)
	{
		flDistance /= 149598000;		// Km in an AU
		if (flDistance > 30000)
		{
			flDistance /= 63239.6717f;	// AU in a LY
			return sprintf("%.2fly", flDistance);
		}

		if (flDistance < 1)
			return sprintf("%.4fau", flDistance);
		else
			return sprintf("%.2fau", flDistance);
	}

	return sprintf("%.2fkm", flDistance);
}

void CSPHUD::Paint(float x, float y, float w, float h)
{
	BaseClass::Paint(x, y, w, h);

	Vector vecUp;
	Vector vecForward;
	GameServer()->GetRenderer()->GetCameraVectors(&vecForward, NULL, &vecUp);

	for (size_t i = 0; i < GameServer()->GetMaxEntities(); i++)
	{
		CBaseEntity* pEntity = CBaseEntity::GetEntity(i);
		if (!pEntity)
			continue;

		CPlanet* pPlanet = dynamic_cast<CPlanet*>(pEntity);
		if (pPlanet)
		{
			CScalableVector vecScalablePlanet = pPlanet->GameData().GetScalableRenderOrigin();

			CScalableFloat flDistance = vecScalablePlanet.Length();
			if (flDistance < pPlanet->GetRadius()*2.0f)
				continue;

			Vector vecPlanet = vecScalablePlanet.GetUnits(SCALE_METER);

			if (vecForward.Dot((vecPlanet).Normalized()) < 0)
				continue;

			Vector vecScreen = GameServer()->GetRenderer()->ScreenPosition(vecPlanet);

			tstring sLabel = pPlanet->GetPlanetName() + " - " + GetStringDistance(flDistance);

			glgui::CLabel::PaintText(sLabel, sLabel.length(), "sans-serif", 16, vecScreen.x + 15, vecScreen.y - 15);
		}

		CSpire* pSpire = dynamic_cast<CSpire*>(pEntity);
		if (pSpire)
		{
			CScalableVector vecScalableSpire = pSpire->GameData().GetScalableRenderOrigin();
			CScalableFloat flDistance = vecScalableSpire.Length();

			if (flDistance < CScalableFloat(100.0f, SCALE_METER))
				continue;

			CPlanet* pSpirePlanet = pSpire->GameData().GetPlanet();
			TAssert(pSpirePlanet);

			CScalableVector vecScalablePlanet = pSpire->GameData().GetPlanet()->GameData().GetScalableRenderOrigin();
			if (vecScalablePlanet.LengthSqr() > CScalableFloat(60.0f, SCALE_MEGAMETER)*CScalableFloat(60.0f, SCALE_MEGAMETER))
				continue;

			Vector vecSpire = vecScalableSpire.GetUnits(SCALE_METER);

			if (vecForward.Dot((vecSpire).Normalized()) < 0)
				continue;

			Vector vecScreen = GameServer()->GetRenderer()->ScreenPosition(vecSpire);

			tstring sLabel = pSpirePlanet->GetPlanetName() + " " + pSpire->GetBaseName() + " - " + GetStringDistance(flDistance);

			glgui::CLabel::PaintText(sLabel, sLabel.length(), "sans-serif", 16, vecScreen.x + 15, vecScreen.y - 15);
		}
	}

	CPlayerCharacter* pLocalPlayer = SPGame()->GetLocalPlayerCharacter();
	CHelperBot* pHelper = pLocalPlayer->GetHelperBot();
	if (pHelper)
	{
		CScalableVector vecScalablePlanet = pHelper->GameData().GetScalableRenderOrigin();

		Vector vecPlanet = vecScalablePlanet.GetUnits(SCALE_METER);

		if (vecForward.Dot((vecPlanet).Normalized()) > 0)
		{
			Vector vecScreen = GameServer()->GetRenderer()->ScreenPosition(vecPlanet);

			tstring sLabel = "Helper-Bot";

			float flFontHeight = glgui::CLabel::GetFontHeight("sans-serif", 16);
			float flTextWidth = glgui::CLabel::GetTextWidth(sLabel, sLabel.length(), "sans-serif", 16);

			vecScreen.x += 15;
			vecScreen.y -= 15;

			if (vecScreen.x < 0)
				vecScreen.x = 0;
			if (vecScreen.y < 0)
				vecScreen.y = 0;
			if (vecScreen.x > glgui::RootPanel()->GetWidth() - flTextWidth)
				vecScreen.x = glgui::RootPanel()->GetWidth() - flTextWidth;
			if (vecScreen.y > glgui::RootPanel()->GetHeight() - flFontHeight)
				vecScreen.y = glgui::RootPanel()->GetHeight() - flFontHeight;

			glgui::CLabel::PaintText(sLabel, sLabel.length(), "sans-serif", 16, vecScreen.x, vecScreen.y);
		}
	}

	// Paint health blocks.
	for (size_t i = 0; i < pLocalPlayer->GetHealth(); i++)
		glgui::CBaseControl::PaintRect(w-20, h-40*(i+1), 10, 30, Color(255, 0, 0, 255), 1, true);

	Debug_Paint();
}

void CSPHUD::PlaceBlockCallback(const tstring& sArgs)
{
	m_ahMenus[MENU_BLOCKS]->CloseMenu();

	item_t eBlock = (item_t)(stoi(sArgs)+1);

	CSPPlayer* pPlayer = SPGame()->GetLocalSPPlayer();

	pPlayer->EnterBlockPlaceMode(eBlock);
}

void CSPHUD::ConstructSpireCallback(const tstring& sArgs)
{
	m_ahMenus[MENU_CONSTRUCTION]->CloseMenu();

	CSPPlayer* pPlayer = SPGame()->GetLocalSPPlayer();

	pPlayer->EnterConstructionMode(STRUCTURE_SPIRE);
}

void CSPHUD::ConstructMineCallback(const tstring& sArgs)
{
	m_ahMenus[MENU_CONSTRUCTION]->CloseMenu();

	CSPPlayer* pPlayer = SPGame()->GetLocalSPPlayer();

	pPlayer->EnterConstructionMode(STRUCTURE_MINE);
}

void CSPHUD::ConstructPalletCallback(const tstring& sArgs)
{
	m_ahMenus[MENU_CONSTRUCTION]->CloseMenu();

	CSPPlayer* pPlayer = SPGame()->GetLocalSPPlayer();

	pPlayer->EnterConstructionMode(STRUCTURE_PALLET);
}

void CSPHUD::Debug_Paint()
{
	Vector vecUp;
	Vector vecForward;
	GameServer()->GetRenderer()->GetCameraVectors(&vecForward, NULL, &vecUp);

	CPlayerCharacter* pLocalCharacter = SPGame()->GetLocalPlayerCharacter();
}
