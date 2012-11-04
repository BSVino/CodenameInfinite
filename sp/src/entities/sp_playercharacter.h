#ifndef SP_PLAYER_CHARACTER_H
#define SP_PLAYER_CHARACTER_H

#include "sp_character.h"

#include "sp_camera.h"

class CHelperBot;
class CDisassembler;

class CPlayerCharacter : public CSPCharacter
{
	REGISTER_ENTITY_CLASS(CPlayerCharacter, CSPCharacter);

public:
								CPlayerCharacter();

public:
	virtual void				Spawn();

	virtual void				Think();
	virtual void				MoveThink();

	virtual bool                CanAttack() const;

	void                        OnWeaponAdded(CBaseWeapon* pWeapon);
	void                        OnWeaponRemoved(CBaseWeapon* pWeapon, bool bWasEquipped);

	void                        BeginDisassembly(CStructure* pStructure);

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

	virtual size_t              MaxInventory() const { return 10; }
	virtual size_t              MaxSlots() const { return INVENTORY_SLOTS; }

	virtual bool                TakesBlocks() { return true; }

	virtual float               MeleeAttackSphereRadius() const { return 1.2f; }
	virtual float               MeleeAttackDamage() const { return 6; }

protected:
	bool						m_bFlying;
	bool						m_bWalkSpeedOverride;
	bool						m_bHyperdrive;

	CEntityHandle<CSPCamera>    m_hCamera;

	double                      m_flNextApproximateElevation;
	double                      m_flApproximateElevation;     // From the center of the planet

	CEntityHandle<CHelperBot>   m_hHelper;
	CEntityHandle<CDisassembler> m_hDisassembler;

	CEntityHandle<CStructure>   m_hDisassemblingStructure;
	double                      m_flDisassemblingStructureStart;

	double                      m_flNextSurfaceCheck;
};

#endif
