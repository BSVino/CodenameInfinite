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

	virtual void                CharacterMovement(class btCollisionWorld*, float flDelta);
	virtual const TMatrix       GetPhysicsTransform() const;
	virtual void                SetPhysicsTransform(const TMatrix& m);

	void						ToggleFlying();
	void						StartFlying();
	void						StopFlying();

	virtual void                EnteredAtmosphere();

	void						SetWalkSpeedOverride(bool bOverride) { m_bWalkSpeedOverride = bOverride; };

	void						EngageHyperdrive() { m_bHyperdrive = true; };
	void						DisengageHyperdrive() { m_bHyperdrive = false; };

	virtual CScalableFloat		CharacterSpeed();

	void						ApproximateElevation();

	virtual inline const CScalableVector	GetGlobalGravity() const;

protected:
	bool						m_bFlying;
	bool						m_bWalkSpeedOverride;
	bool						m_bHyperdrive;

	CEntityHandle<CSPCamera>    m_hCamera;

	double                      m_flNextApproximateElevation;
	double                      m_flApproximateElevation;     // From the center of the planet

	size_t                      m_iCurrentPhysicsChunk;
	DoubleVector                m_vecChunkOffset;
};

#endif
