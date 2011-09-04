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

	virtual CScalableMatrix		GetScalableRenderTransform() const;
	virtual CScalableVector		GetScalableRenderOrigin() const;

	virtual Matrix4x4			GetRenderTransform() const;
	virtual Vector				GetRenderOrigin() const;

	CPlanet*					GetNearestPlanet(findplanet_t eFindPlanet = FINDPLANET_INATMOSPHERE);
	CPlanet*					GetNearestPlanet(findplanet_t eFindPlanet = FINDPLANET_INATMOSPHERE) const;
	CPlanet*					FindNearestPlanet() const;

	virtual TVector				GetUpVector() const;

	void						LockViewToPlanet();

	void						StandOnNearestPlanet();

	virtual TFloat				GetBoundingRadius() const { return 2.0f; };
	virtual CScalableFloat		EyeHeight() const;
	virtual CScalableFloat		CharacterSpeed();

protected:
	CNetworkedHandle<CPlanet>	m_hNearestPlanet;
	float						m_flNextPlanetCheck;

	float						m_flLastEnteredAtmosphere;
	float						m_flRollFromSpace;
};

#endif
