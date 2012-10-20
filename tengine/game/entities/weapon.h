#pragma once

#include <tengine/game/entities/baseentity.h>

class CBaseWeapon : public CBaseEntity
{
	REGISTER_ENTITY_CLASS(CBaseWeapon, CBaseEntity);

public:
	virtual float                   MeleeAttackTime() const { return 0.3f; }
	virtual float                   MeleeAttackDamage() const { return 50; }
	virtual float                   MeleeAttackSphereRadius() const { return 40.0f; }
	virtual const TVector           MeleeAttackSphereCenter() const { return GetGlobalCenter(); }
};

#endif
