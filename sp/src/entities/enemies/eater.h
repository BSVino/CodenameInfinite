#pragma once

#include "entities/sp_character.h"

class CEater : public CSPCharacter
{
	REGISTER_ENTITY_CLASS(CEater, CSPCharacter);

public:
	void        Precache();
	void        Spawn();
	void        Think();

	bool        ShouldRender() const;
	void        PostRender() const;

	virtual float                   MeleeAttackTime() const { return 0.5f; }
	virtual float                   MeleeAttackDamage() const { return 1; }

	CScalableFloat  CharacterSpeed();

	collision_group_t           GetCollisionGroup() const { return CG_CHARACTER_CLIP; }
};
