#ifndef SP_PLAYER_H
#define SP_PLAYER_H

#include <tengine/game/entities/player.h>

class CSPPlayer : public CPlayer
{
	REGISTER_ENTITY_CLASS(CSPPlayer, CPlayer);

public:
	virtual void					MouseMotion(int x, int y);
	virtual void					KeyPress(int c);
	virtual void					KeyRelease(int c);

	class CSPCharacter*				GetSPCharacter();
};

#endif
