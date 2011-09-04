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
	virtual void					MoveThink();

	void							SetControllingPlayer(CPlayer* pCharacter);
	CPlayer*						GetControllingPlayer() const;

	virtual TFloat					EyeHeight() { return 180.0f; }
	virtual TFloat					CharacterSpeed() { return 80.0f; }

	virtual bool					ShouldCollide() const { return true; }

	CBaseEntity*					GetGroundEntity() const { return m_hGround; }
	void							SetGroundEntity(CBaseEntity* pEntity) { m_hGround = pEntity; }
	virtual void					FindGroundEntity();

protected:
	CNetworkedHandle<CPlayer>		m_hControllingPlayer;

	CNetworkedHandle<CBaseEntity>	m_hGround;

	Vector							m_vecGoalVelocity;
	Vector							m_vecMoveVelocity;
};

#endif
