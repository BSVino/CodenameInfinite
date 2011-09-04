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
	virtual void				Think();

	CPlanet*					GetNearestPlanet(findplanet_t eFindPlanet = FINDPLANET_INATMOSPHERE);
	CPlanet*					FindNearestPlanet();

	virtual TVector				GetUpVector();

	void						LockViewToPlanet();

	void						StandOnNearestPlanet();

	virtual CScalableFloat		EyeHeight();
	virtual CScalableFloat		CharacterSpeed();

	virtual inline CScalableVector	GetGlobalGravity() const;
	virtual void				FindGroundEntity();

protected:
	CNetworkedHandle<CPlanet>	m_hNearestPlanet;
	float						m_flNextPlanetCheck;

	float						m_flLastEnteredAtmosphere;
	float						m_flRollFromSpace;
};

#endif
