#ifndef SP_PLAYER_CHARACTER_H
#define SP_PLAYER_CHARACTER_H

#include "sp_character.h"

#include "sp_camera.h"

class CPlayerCharacter : public CSPCharacter
{
	REGISTER_ENTITY_CLASS(CPlayerCharacter, CSPCharacter);

public:
								CPlayerCharacter();

public:
	virtual void				Spawn();

	virtual void				Think();
	virtual void				MoveThink();

	void						ToggleFlying();
	void						StartFlying();
	void						StopFlying();

	void						EngageHyperdrive() { m_bHyperdrive = true; };
	void						DisengageHyperdrive() { m_bHyperdrive = false; };

	virtual CScalableFloat		CharacterSpeed();

	virtual inline const CScalableVector	GetGlobalGravity() const;

protected:
	bool						m_bFlying;
	bool						m_bHyperdrive;

	CEntityHandle<CSPCamera>    m_hCamera;
};

#endif
