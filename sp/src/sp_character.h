#ifndef SP_CHARACTER_H
#define SP_CHARACTER_H

#include <tengine/game/entities/character.h>

#include "sp_common.h"
#include "sp_entity.h"

class CPlanet;

typedef enum {
	FINDPLANET_ANY,
	FINDPLANET_CLOSEORBIT,
	FINDPLANET_INATMOSPHERE,
} findplanet_t;

class CSPCharacter : public CCharacter
{
	REGISTER_ENTITY_CLASS(CSPCharacter, CCharacter);

public:
								CSPCharacter();

public:
	virtual void				Spawn();
	virtual void				Think();

	virtual const CScalableMatrix GetScalableRenderTransform() const;
	virtual const CScalableVector GetScalableRenderOrigin() const;

	virtual const Matrix4x4     GetRenderTransform() const;
	virtual const Vector        GetRenderOrigin() const;

	CPlanet*					GetNearestPlanet(findplanet_t eFindPlanet = FINDPLANET_INATMOSPHERE);
	CPlanet*					GetNearestPlanet(findplanet_t eFindPlanet = FINDPLANET_INATMOSPHERE) const;
	CPlanet*					FindNearestPlanet() const;

	virtual const Vector        GetUpVector() const;
	virtual const Vector        GetLocalUpVector() const;

	void						LockViewToPlanet();

	void						StandOnNearestPlanet();

	virtual void                EnteredAtmosphere() {};

	virtual const TFloat        GetBoundingRadius() const { return 2.0f; };
	virtual CScalableFloat		EyeHeight() const;
	virtual TFloat				JumpStrength();
	virtual CScalableFloat		CharacterSpeed();

protected:
	CNetworkedHandle<CPlanet>	m_hNearestPlanet;
	double                      m_flNextPlanetCheck;

	double                      m_flLastEnteredAtmosphere;
	float						m_flRollFromSpace;
};

#endif
