#ifndef SP_GAME_H
#define SP_GAME_H

#include <tengine/game/entities/game.h>

class CSPGame : public CGame
{
	REGISTER_ENTITY_CLASS(CSPGame, CGame);

public:
	CSPGame();

public:
	virtual void			Precache();

	virtual void			SetupGame(tstring sType);

	virtual void			Think();

	virtual bool            TakesDamageFrom(CBaseEntity* pVictim, CBaseEntity* pAttacker);

	class CSPPlayer*        GetLocalSPPlayer();
	class CPlayerCharacter*	GetLocalPlayerCharacter();
	class CSPRenderer*		GetSPRenderer();

private:
	bool                    m_bWaitingForTerrainToGenerate;
};

inline class CSPGame* SPGame()
{
	CGame* pGame = Game();
	if (!pGame)
		return NULL;

	return static_cast<CSPGame*>(pGame);
}

#endif
