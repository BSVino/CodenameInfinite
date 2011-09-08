#ifndef SP_PLAYER_CHARACTER_H
#define SP_PLAYER_CHARACTER_H

#include "sp_character.h"

class CPlayerCharacter : public CSPCharacter
{
	REGISTER_ENTITY_CLASS(CPlayerCharacter, CSPCharacter);

public:
								CPlayerCharacter();

public:
	virtual void				Think();
	virtual void				MoveThink();

	void						ToggleFlying();
	void						StartFlying();
	void						StopFlying();

	void						SetWalkSpeedOverride(bool bOverride) { m_bWalkSpeedOverride = bOverride; };

	void						EngageHyperdrive() { m_bHyperdrive = true; };
	void						DisengageHyperdrive() { m_bHyperdrive = false; };

	virtual CScalableFloat		CharacterSpeed();

	virtual inline CScalableVector	GetGlobalGravity() const;
	virtual void				FindGroundEntity();

protected:
	bool						m_bFlying;
	bool						m_bWalkSpeedOverride;
	bool						m_bHyperdrive;
};

#endif
