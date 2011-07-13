#ifndef TINKER_CHARACTER_H
#define TINKER_CHARACTER_H

#include <tengine/game/baseentity.h>

class CPlayer;

typedef enum
{
	MOVE_FORWARD,
	MOVE_BACKWARD,
	MOVE_LEFT,
	MOVE_RIGHT,
} movetype_t;

class CCharacter : public CBaseEntity
{
	REGISTER_ENTITY_CLASS(CCharacter, CBaseEntity);

public:
	virtual void					Spawn();
	virtual void					Think();

	void							Move(movetype_t);
	void							StopMove(movetype_t);

	void							SetControllingPlayer(CPlayer* pCharacter);
	CPlayer*						GetControllingPlayer() const;

	virtual float					CharacterSpeed() { return 80; }

protected:
	CNetworkedHandle<CPlayer>		m_hControllingPlayer;

	Vector							m_vecGoalVelocity;
	Vector							m_vecMoveVelocity;
};

#endif
