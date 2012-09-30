#pragma once

#include "bot.h"

class CHelperBot : public CBot
{
	REGISTER_ENTITY_CLASS(CHelperBot, CBot);

public:
	void        Precache();
	void        Spawn();
	void        Think();

	void        TeleportToPlayer();

	void        SetPlayerCharacter(class CPlayerCharacter* pPlayer);

	bool        ShouldRender() const;
	void        PostRender() const;

private:
	CEntityHandle<CPlayerCharacter>    m_hPlayer;
};
