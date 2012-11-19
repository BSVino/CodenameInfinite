#pragma once

#include <game/entities/weapon.h>

#include <voxel/ivector.h>
#include <planet/treeaddress.h>

class CStructure;

class CDisassembler : public CBaseWeapon
{
	REGISTER_ENTITY_CLASS(CDisassembler, CBaseWeapon);

public:
	virtual void                Precache();

	virtual void                Spawn();

	virtual void                Think();

	virtual void                DrawViewModel(class CGameRenderingContext* pContext);

	virtual void                OwnerMouseInput(int iButton, int iState);

	void                        BeginDisassembly(CStructure* pStructure);
	void                        BeginDisassembly(const IVector& vecBlock);
	void                        BeginDisassembly(const TVector& vecGround);
	void                        BeginDisassembly(const CTreeAddress& oTree);
	void                        BeginDisassembly();
	void                        EndDisassembly();
	bool                        IsDisassembling() const { return m_bDisassembling; }
	void                        RestartParticles();

	virtual void                MeleeAttack();

	virtual float               MeleeAttackTime() const { return 0.5f; }
	virtual float               MeleeAttackSphereRadius() const { return 1.2f; }
	virtual float               MeleeAttackDamage() const { return 6; }

	virtual float               Range() const { return 4; }

private:
	bool                        m_bDisassembling;

	CEntityHandle<CStructure>   m_hDisassemblingStructure;
	IVector                     m_vecDisassemblingBlock;
	TVector                     m_vecDisassemblingGround;
	CTreeAddress                m_oDisassemblingTree;
	double                      m_flDisassemblingStart;
};
