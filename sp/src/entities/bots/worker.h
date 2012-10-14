#pragma once

#include "bot.h"

class CWorkerBot : public CBot
{
	REGISTER_ENTITY_CLASS(CWorkerBot, CBot);

public:
	void        Precache();
	void        Spawn();
	void        Think();

	bool        ShouldRender() const;
	void        PostRender() const;

	bool        TakesBlocks() const { return true; }

	CScalableFloat  CharacterSpeed();
};
