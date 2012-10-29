#include "bot.h"

#include <tengine/renderer/game_renderer.h>

#include "entities/sp_playercharacter.h"
#include "ui/command_menu.h"

REGISTER_ENTITY(CBot);

NETVAR_TABLE_BEGIN(CBot);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CBot);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CSPPlayer, m_hOwner);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CBot);
INPUTS_TABLE_END();

void CBot::OnUse(CBaseEntity* pUser)
{
	CPlayerCharacter* pPlayerCharacter = dynamic_cast<CPlayerCharacter*>(pUser);
	if (pPlayerCharacter)
	{
		CCommandMenu* pMenu = GameData().CreateCommandMenu(pPlayerCharacter);
		SetupMenuButtons();

		if (!pMenu->GetNumActiveButtons())
			GameData().CloseCommandMenu();
	}
}

void CBot::Think()
{
	BaseClass::Think();

	if (GameData().GetCommandMenu())
	{
		if (GameData().GetCommandMenu()->WantsToClose())
			GameData().CloseCommandMenu();
		else
			GameData().GetCommandMenu()->Think();
	}
}

void CBot::PostRender() const
{
	BaseClass::PostRender();

	if (!GameServer()->GetRenderer()->IsRenderingTransparent())
		return;

	if (GameData().GetCommandMenu())
		GameData().GetCommandMenu()->Render();
}
