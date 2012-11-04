#pragma once

#include <game/entities/weapon.h>

class CDisassembler : public CBaseWeapon
{
	REGISTER_ENTITY_CLASS(CDisassembler, CBaseWeapon);

public:
	virtual void                Precache();

	virtual void                Spawn();

	virtual void                DrawViewModel(class CGameRenderingContext* pContext);

	virtual void                BeginDisassembly();
	virtual void                EndDisassembly();

	virtual void                MeleeAttack();

	virtual float               MeleeAttackTime() const { return 0.5f; }
	virtual float               MeleeAttackSphereRadius() const { return 1.2f; }
	virtual float               MeleeAttackDamage() const { return 6; }

private:
	bool                        m_bDisassembling;
};
