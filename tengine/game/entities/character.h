#ifndef TINKER_CHARACTER_H
#define TINKER_CHARACTER_H

#include <tengine/game/entities/baseentity.h>

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
									CCharacter();

public:
	virtual void					Spawn();
	virtual void					Think();
	virtual void                    Simulate();

	void							Move(movetype_t);
	void							StopMove(movetype_t);
	virtual const TVector			GetGoalVelocity();
	virtual void					MoveThink();
	virtual void					MoveThink_NoClip();
	virtual void					Jump();
	virtual const TMatrix           GetMovementVelocityTransform() const;

	virtual void                    CharacterMovement(class btCollisionWorld*, float flDelta);

	virtual void					SetNoClip(bool bOn);
	virtual bool					GetNoClip() const { return m_bNoClip; }

	virtual bool					CanAttack() const;
	virtual void					Attack();
	virtual bool					IsAttacking() const;

	virtual void					MoveToPlayerStart();

	virtual void					PostRender() const;
	virtual void					ShowPlayerVectors() const;

	void							SetControllingPlayer(CPlayer* pCharacter);
	CPlayer*						GetControllingPlayer() const;

	virtual TFloat					EyeHeight() const { return 180.0f; }
	virtual TFloat					BaseCharacterSpeed() { return 80.0f; }
	virtual TFloat					CharacterAcceleration() { return 4.0f; }
	virtual TFloat					JumpStrength() { return 150.0f; }
	virtual TFloat					CharacterSpeed();

	virtual float					AttackTime() const { return 0.3f; }
	virtual float					AttackDamage() const { return 50; }
	virtual float					AttackSphereRadius() const { return 40.0f; }
	virtual const TVector			AttackSphereCenter() const { return GetGlobalCenter(); }

	virtual bool					ShouldCollide() const { return false; }

	virtual const EAngle			GetThirdPersonCameraAngles() const { return GetViewAngles(); }

	CBaseEntity*					GetGroundEntity() const { return m_hGround; }
	virtual void                    SetGroundEntity(CBaseEntity* pEntity);
	virtual void                    SetGroundEntityExtra(size_t iExtra);

	TFloat							GetMaxStepHeight() const { return m_flMaxStepSize; }

	double                          GetLastSpawn() const { return m_flLastSpawn; }

	virtual bool					UsePhysicsModelForController() const { return false; }

protected:
	CNetworkedHandle<CPlayer>		m_hControllingPlayer;

	CNetworkedHandle<CBaseEntity>	m_hGround;

	bool							m_bNoClip;

	bool							m_bTransformMoveByView;
	TVector							m_vecGoalVelocity;
	TVector							m_vecMoveVelocity;
	double							m_flMoveSimulationTime;

	double							m_flLastAttack;
	double                          m_flLastSpawn;

	TFloat							m_flMaxStepSize;
};

#endif
