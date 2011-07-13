#ifndef SP_CHARACTER_H
#define SP_CHARACTER_H

#include <tengine/game/entities/character.h>

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
	CPlanet*					GetNearestPlanet(findplanet_t eFindPlanet = FINDPLANET_INATMOSPHERE);
	CPlanet*					FindNearestPlanet();

	virtual Vector				GetUpVector();

	void						LockViewToPlanet();

	void						StandOnNearestPlanet();

	virtual float				EyeHeight();
	virtual float				CharacterSpeed();

protected:
	CNetworkedHandle<CPlanet>	m_hNearestPlanet;
	float						m_flNextPlanetCheck;
};

#endif
