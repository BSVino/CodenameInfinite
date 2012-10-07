#ifndef SP_PLAYER_CHARACTER_H
#define SP_PLAYER_CHARACTER_H

#include "sp_character.h"

#include "sp_camera.h"

class CHelperBot;

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
	bool                        IsFlying() const { return m_bFlying; }

	virtual void                EnteredAtmosphere();

	void						SetWalkSpeedOverride(bool bOverride) { m_bWalkSpeedOverride = bOverride; };

	void						EngageHyperdrive() { m_bHyperdrive = true; };
	void						DisengageHyperdrive() { m_bHyperdrive = false; };

	virtual CScalableFloat		CharacterSpeed();

	void						ApproximateElevation();
	double                      GetApproximateElevation() const { return m_flApproximateElevation; }

	virtual inline const CScalableVector	GetGlobalGravity() const;

	CHelperBot*                 GetHelperBot();
	virtual bool                ApplyGravity() const { return !m_bFlying; }

protected:
	bool						m_bFlying;
	bool						m_bWalkSpeedOverride;
	bool						m_bHyperdrive;

	CEntityHandle<CSPCamera>    m_hCamera;

	double                      m_flNextApproximateElevation;
	double                      m_flApproximateElevation;     // From the center of the planet

	CEntityHandle<CHelperBot>   m_hHelper;
};

#endif
