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
#include "entities/structures/pallet.h"

// For debug shit
#include "planet/planet_terrain.h"
#include "planet/terrain_lumps.h"
#include "planet/terrain_chunks.h"

#include "hudmenu.h"

CSPHUD::CSPHUD()
	: CHUDViewport()
{
}

void CSPHUD::BuildMenus()
{
	for (int i = 0; i < MENU_TOTAL; i++)
	{
		if (!m_ahMenus[i])
			continue;

		m_ahMenus[i]->SetVisible(false);

		for (size_t k = 0; k < m_ahMenus[i]->GetNumSubmenus(); k++)
		{
			glgui::CControl<glgui::CMenu> pMenu = m_ahMenus[i]->GetSubmenu(k);
			if (!pMenu)
				continue;

			pMenu->SetVisible(false);
		}
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
		if (!m_ahMenus[MENU_EQUIP])
			m_ahMenus[MENU_EQUIP] = glgui::RootPanel()->AddControl(new CHUDMenu(MENU_EQUIP, sprintf("%d. Equip", MENU_EQUIP+1)));

		m_ahMenus[MENU_EQUIP]->SetVisible(true);

		size_t iMenu = 0;
		for (size_t i = 1; i < ITEM_BLOCKS_TOTAL; i++)
		{
			if (pCharacter->GetInventory((item_t)i))
			{
				if (!m_ahMenus[MENU_EQUIP]->GetSubmenu(iMenu))
					m_ahMenus[MENU_EQUIP]->AddSubmenu(new CHUDMenu(true), this, PlaceBlock);

				CHUDMenu* pMenu = m_ahMenus[MENU_EQUIP]->GetSubmenu(iMenu).DowncastStatic<CHUDMenu>();
				pMenu->SetMenuListener(this, PlaceBlock);
				pMenu->SetIndex(i-1);
				pMenu->SetText(sprintf("%d. " + tstring(GetItemName((item_t)i)) + " (x%d)", i, pCharacter->GetInventory((item_t)i)));
				pMenu->SetVisible(true);

				iMenu++;
			}
		}
	}

	CSpire* pSpire = pCharacter->GetNearestSpire();
	if (pSpire)
	{
		int aiBlocks[ITEM_BLOCKS_TOTAL];
		bool bHasBlocks = false;
		memset(aiBlocks, 0, sizeof(aiBlocks));

		for (size_t i = 0; i < pSpire->GetStructures().size(); i++)
		{
			CPallet* pPallet = dynamic_cast<CPallet*>(pSpire->GetStructures()[i].GetPointer());
			if (!pPallet)
				continue;

			if (pPallet->GetBlockQuantity())
			{
				aiBlocks[pPallet->GetBlockType()] += pPallet->GetBlockQuantity();
				bHasBlocks = true;
			}
		}

		if (bHasBlocks)
		{
			if (!m_ahMenus[MENU_BLOCKS])
				m_ahMenus[MENU_BLOCKS] = glgui::RootPanel()->AddControl(new CHUDMenu(MENU_BLOCKS, sprintf("%d. Blocks", MENU_BLOCKS+1)));

			m_ahMenus[MENU_BLOCKS]->SetVisible(true);

			size_t iMenu = 0;
			for (size_t i = 1; i < ITEM_BLOCKS_TOTAL; i++)
			{
				if (!m_ahMenus[MENU_BLOCKS]->GetSubmenu(iMenu))
					m_ahMenus[MENU_BLOCKS]->AddSubmenu(new CHUDMenu(true), this, DesignateBlock);

				CHUDMenu* pMenu = m_ahMenus[MENU_BLOCKS]->GetSubmenu(iMenu).DowncastStatic<CHUDMenu>();
				pMenu->SetMenuListener(this, DesignateBlock);
				pMenu->SetIndex(i-1);
				pMenu->SetText(sprintf("%d. " + tstring(GetItemName((item_t)i)) + " (x%d)", i, aiBlocks[i]));
				pMenu->SetVisible(true);

				iMenu++;
			}
		}
	}

	{
		if (!m_ahMenus[MENU_CONSTRUCTION])
			m_ahMenus[MENU_CONSTRUCTION] = glgui::RootPanel()->AddControl(new CHUDMenu(MENU_CONSTRUCTION, sprintf("%d. Construction", MENU_CONSTRUCTION+1)));

		m_ahMenus[MENU_CONSTRUCTION]->SetVisible(true);

		size_t iMenu = 0;
		if (pCharacter->CanBuildStructure(STRUCTURE_SPIRE))
		{
			if (!m_ahMenus[MENU_CONSTRUCTION]->GetSubmenu(iMenu))
				m_ahMenus[MENU_CONSTRUCTION]->AddSubmenu(new CHUDMenu(true), this, ConstructSpire);

			CHUDMenu* pMenu = m_ahMenus[MENU_CONSTRUCTION]->GetSubmenu(iMenu).DowncastStatic<CHUDMenu>();
			pMenu->SetMenuListener(this, ConstructSpire);
			pMenu->SetIndex(0);
			pMenu->SetText("1. Spire");
			pMenu->SetVisible(true);

			iMenu++;
		}

		if (pCharacter->CanBuildStructure(STRUCTURE_MINE))
		{
			if (!m_ahMenus[MENU_CONSTRUCTION]->GetSubmenu(iMenu))
				m_ahMenus[MENU_CONSTRUCTION]->AddSubmenu(new CHUDMenu(true), this, ConstructMine);

			CHUDMenu* pMenu = m_ahMenus[MENU_CONSTRUCTION]->GetSubmenu(iMenu).DowncastStatic<CHUDMenu>();
			pMenu->SetMenuListener(this, ConstructMine);
			pMenu->SetIndex(1);
			pMenu->SetText("2. Mine");
			pMenu->SetVisible(true);

			iMenu++;
		}

		if (pCharacter->CanBuildStructure(STRUCTURE_PALLET))
		{
			if (!m_ahMenus[MENU_CONSTRUCTION]->GetSubmenu(iMenu))
				m_ahMenus[MENU_CONSTRUCTION]->AddSubmenu(new CHUDMenu(true), this, ConstructPallet);

			CHUDMenu* pMenu = m_ahMenus[MENU_CONSTRUCTION]->GetSubmenu(iMenu).DowncastStatic<CHUDMenu>();
			pMenu->SetMenuListener(this, ConstructPallet);
			pMenu->SetIndex(2);
			pMenu->SetText("3. Pallet");
			pMenu->SetVisible(true);

			iMenu++;
		}

		if (!iMenu)
			m_ahMenus[MENU_CONSTRUCTION]->SetVisible(false);
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

	CPlayerCharacter* pLocalPlayer = SPGame()->GetLocalPlayerCharacter();

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

			if (pLocalPlayer && pLocalPlayer->GameData().GetPlanet())
			{
				Vector vecToPlayersPlanet = (pLocalPlayer->GameData().GetPlanet()->GetGlobalOrigin() - pLocalPlayer->GetGlobalOrigin()).Normalized();
				Vector vecToThisPlanet = (pPlanet->GetGlobalOrigin() - pLocalPlayer->GetGlobalOrigin()).Normalized();

				if (vecToPlayersPlanet.Dot(vecToThisPlanet) > 0)
					continue;
			}

			Vector vecScreen = GameServer()->GetRenderer()->ScreenPosition(vecPlanet);

			tstring sLabel = pPlanet->GetPlanetName() + " - " + GetStringDistance(flDistance);

			glgui::CLabel::PaintText(sLabel, sLabel.length(), "sans-serif", 16, vecScreen.x + 15, vecScreen.y - 15);
		}

		CSpire* pSpire = dynamic_cast<CSpire*>(pEntity);
		if (pSpire)
		{
			CScalableVector vecScalableSpire = pSpire->GameData().GetScalableRenderOrigin();
			CScalableFloat flDistance = vecScalableSpire.Length();

			if (flDistance < CScalableFloat(50.0f, SCALE_METER))
				continue;

			CPlanet* pSpirePlanet = pSpire->GameData().GetPlanet();
			TAssert(pSpirePlanet);

			CScalableVector vecScalablePlanet = pSpire->GameData().GetPlanet()->GameData().GetScalableRenderOrigin();
			if (vecScalablePlanet.LengthSqr() > CScalableFloat(60.0f, SCALE_MEGAMETER)*CScalableFloat(60.0f, SCALE_MEGAMETER))
				continue;

			Vector vecSpire = vecScalableSpire.GetUnits(SCALE_METER);

			if (vecForward.Dot((vecSpire).Normalized()) < 0)
				continue;

			if (pLocalPlayer && pSpire->GameData().GetPlanet())
			{
				Vector vecToPlayer = (pLocalPlayer->GetGlobalOrigin() - pSpirePlanet->GetGlobalOrigin()).Normalized();
				Vector vecToSpire = (pSpire->GetGlobalOrigin() - pSpirePlanet->GetGlobalOrigin()).Normalized();
				TFloat flDistanceToSurface = (pLocalPlayer->GetGlobalOrigin() - pSpirePlanet->GetGlobalOrigin()).Length() - pSpirePlanet->GetRadius();

				if (vecToPlayer.Dot(vecToSpire) < (float)RemapValClamped(flDistanceToSurface, pSpirePlanet->GetAtmosphereThickness(), TFloat(1.0, SCALE_MEGAMETER), 0.99, 0.0))
					continue;
			}

			Vector vecScreen = GameServer()->GetRenderer()->ScreenPosition(vecSpire);

			tstring sLabel = pSpirePlanet->GetPlanetName() + " " + pSpire->GetBaseName() + " - " + GetStringDistance(flDistance);

			glgui::CLabel::PaintText(sLabel, sLabel.length(), "sans-serif", 16, vecScreen.x + 15, vecScreen.y - 15);
		}
	}

	CHelperBot* pHelper = pLocalPlayer->GetHelperBot();
	if (pHelper && !pHelper->GameData().GetCommandMenu())
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

	Debug_Paint(w, h);
}

void CSPHUD::PlaceBlockCallback(const tstring& sArgs)
{
	m_ahMenus[MENU_EQUIP]->CloseMenu();

	item_t eBlock = (item_t)(stoi(sArgs)+1);

	CSPPlayer* pPlayer = SPGame()->GetLocalSPPlayer();

	pPlayer->EnterBlockPlaceMode(eBlock);
}

void CSPHUD::DesignateBlockCallback(const tstring& sArgs)
{
	m_ahMenus[MENU_BLOCKS]->CloseMenu();

	item_t eBlock = (item_t)(stoi(sArgs)+1);

	CSPPlayer* pPlayer = SPGame()->GetLocalSPPlayer();

	pPlayer->EnterBlockDesignateMode(eBlock);
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

CVar terrain_debug("terrain_debug", "off");

void CSPHUD::Debug_Paint(float w, float h)
{
	CPlayerCharacter* pLocalCharacter = SPGame()->GetLocalPlayerCharacter();

	if (terrain_debug.GetBool() && pLocalCharacter && pLocalCharacter->GameData().GetPlanet())
	{
		CPlanet* pPlanet = pLocalCharacter->GameData().GetPlanet();
		size_t iShells = 0;
		for (size_t i = 0; i < 6; i++)
		{
			if (pPlanet->GetTerrain(i)->IsShell2Done())
				iShells++;
		}

		tstring sPlanet = sprintf("Planet shells: %d/6", iShells);
		glgui::CLabel::PaintText(sPlanet, -1, "sans-serif", 10, w-250, 15);

		const CTerrainLumpManager* pLumpManager = pPlanet->GetLumpManager();
		size_t iActiveLumps = 0;
		size_t iGeneratingLumps = 0;
		for (size_t i = 0; i < pLumpManager->GetNumLumps(); i++)
		{
			CTerrainLump* pLump = pLumpManager->GetLump(i);
			if (pLump)
			{
				iActiveLumps++;

				if (pLump->IsGeneratingLowRes())
					iGeneratingLumps++;
			}
		}

		tstring sLumps = sprintf("Lumps: %d slots, %d active, %d generating", pLumpManager->GetNumLumps(), iActiveLumps, iGeneratingLumps);
		glgui::CLabel::PaintText(sLumps, -1, "sans-serif", 10, w-250, 30);

		const CTerrainChunkManager* pChunkManager = pPlanet->GetChunkManager();
		size_t iActiveChunks = 0;
		size_t iGeneratingChunks = 0;
		for (size_t i = 0; i < pChunkManager->GetNumChunks(); i++)
		{
			CTerrainChunk* pChunk = pChunkManager->GetChunk(i);
			if (pChunk)
			{
				iActiveChunks++;

				if (pChunk->IsGeneratingTerrain())
					iGeneratingChunks++;
			}
		}

		tstring sChunks = sprintf("Chunks: %d slots, %d active, %d generating", pChunkManager->GetNumChunks(), iActiveChunks, iGeneratingChunks);
		glgui::CLabel::PaintText(sChunks, -1, "sans-serif", 10, w-250, 45);
	}
}
