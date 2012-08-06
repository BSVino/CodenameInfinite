#ifndef SP_GAME_H
#define SP_GAME_H

#include <tengine/game/entities/game.h>

class CSPGame : public CGame
{
	REGISTER_ENTITY_CLASS(CSPGame, CGame);

public:
	virtual void			Precache();

	virtual void			SetupGame(tstring sType);

	virtual void			Think();

	class CSPCharacter*		GetLocalPlayerCharacter();
	class CSPRenderer*		GetSPRenderer();
};

inline class CSPGame* SPGame()
{
	CGame* pGame = Game();
	if (!pGame)
		return NULL;

	return static_cast<CSPGame*>(pGame);
}

#endif
