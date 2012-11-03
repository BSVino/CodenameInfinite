#include "bot.h"

#include <tengine/renderer/game_renderer.h>

#include "entities/sp_playercharacter.h"
#include "entities/structures/spire.h"
#include "ui/command_menu.h"

REGISTER_ENTITY(CBot);

NETVAR_TABLE_BEGIN(CBot);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CBot);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CSPPlayer, m_hOwner);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CSpire, m_hSpire);
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

		if (pMenu->GetNumActiveButtons())
		{
			if (pPlayerCharacter->GetControllingPlayer())
				pPlayerCharacter->GetControllingPlayer()->Instructor_LessonLearned("order-bots");
		}
		else
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

void CBot::SetSpire(CSpire* pSpire)
{
	m_hSpire = pSpire;

	if (pSpire)
		pSpire->AddUnit(this);
}

CSpire* CBot::GetSpire() const
{
	return m_hSpire;
}
