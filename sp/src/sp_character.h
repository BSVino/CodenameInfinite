#ifndef SP_CHARACTER_H
#define SP_CHARACTER_H

#include <tengine/game/entities/character.h>

class CPlanet;

class CSPCharacter : public CCharacter
{
	REGISTER_ENTITY_CLASS(CSPCharacter, CCharacter);

public:
	CPlanet*					GetNearestPlanet();

	virtual Vector				GetUpVector();

	void						LockViewToPlanet();

	virtual float				CharacterSpeed();

protected:
	CNetworkedHandle<CPlanet>	m_hNearestPlanet;
};

#endif
