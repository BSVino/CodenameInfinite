#ifndef SP_CHARACTER_H
#define SP_CHARACTER_H

#include <tengine/game/entities/character.h>

#include "planet/planet.h"

#include "sp_common.h"
#include "sp_entity.h"
#include "sp_player.h"
#include "items/items.h"

class CPlanet;

typedef enum {
	FINDPLANET_ANY,
	FINDPLANET_CLOSEORBIT,
	FINDPLANET_INATMOSPHERE,
} findplanet_t;

typedef enum
{
	TASK_NONE = 0,
	TASK_BUILD,
	TASK_MINE,
	TASK_ATTACK,
	TASK_FOLLOWME,
	TASK_TOTAL,
} task_t;

const tstring& TaskToString(task_t eTask);

#define INVENTORY_SLOTS 12

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

	virtual bool                ShouldRender() const;
	virtual const Matrix4x4     GetRenderTransform() const;
	virtual const Vector        GetRenderOrigin() const;
	virtual void                ModifyContext(class CRenderingContext* pContext) const;

	virtual bool                CanPickUp(class CPickup* pPickup) const;
	void                        PickUp(class CPickup* pPickup);

	bool                        HasBlocks() const;
	size_t                      GetInventory(item_t eItem) const;

	bool                        PlaceBlock(item_t eItem, const CScalableVector& vecLocal);
	bool                        PlaceBlock(item_t eItem, const IVector& vecBlock);

	virtual const TMatrix       GetMovementVelocityTransform() const;
	virtual void                CharacterMovement(class btCollisionWorld*, float flDelta);
	virtual const Matrix4x4     GetPhysicsTransform() const;
	virtual void                SetPhysicsTransform(const Matrix4x4& m);
	virtual void                PostSetLocalTransform(const TMatrix& m);
	virtual void                SetGroundEntity(CBaseEntity* pEntity);
	virtual void                SetGroundEntityExtra(size_t iExtra);

	// AI stuffs
	virtual void                TaskThink();
	virtual bool                MoveTo(CBaseEntity* pTarget, float flDistance=3); // return true if I'm there
	virtual bool                MoveTo(const TVector& vecTarget, float flDistance=3); // return true if I'm there
	class CStructure*           FindNearestBuildStructure() const;
	const IVector               FindNearbyDesignation(CSpire* pSpire) const;
	class CPallet*              FindNearestPallet(item_t eBlock) const;
	class CMine*                FindNearestMine() const;
	CPickup*                    FindNearbyPickup() const;
	CBaseEntity*                FindBestEnemy() const;

	void                        SetTask(task_t eTask);
	task_t                      GetTask() { return m_eTask; }

	CPlanet*					GetNearestPlanet(findplanet_t eFindPlanet = FINDPLANET_INATMOSPHERE);
	CPlanet*					GetNearestPlanet(findplanet_t eFindPlanet = FINDPLANET_INATMOSPHERE) const;
	CPlanet*					FindNearestPlanet() const;

	CSpire*                     GetNearestSpire() const;
	CSpire*                     FindNearestSpire() const;
	void                        ClearNearestSpire();

	virtual const Vector        GetUpVector() const;
	virtual const Vector        GetLocalUpVector() const;

	void						LockViewToPlanet();

	void						StandOnNearestPlanet();

	virtual void                EnteredAtmosphere() {};

	virtual const TFloat        GetBoundingRadius() const { return 2.0f; };
	virtual CScalableFloat		EyeHeight() const;
	virtual TFloat				JumpStrength();
	virtual CScalableFloat		CharacterSpeed();
	virtual double              GetLastEnteredAtmosphere() const { return m_flLastEnteredAtmosphere; }
	virtual float               GetAtmosphereLerpTime() const { return 1; }
	virtual float               GetAtmosphereLerp() const { return 0.3f; }
	virtual bool                ApplyGravity() const { return true; }

	virtual const TVector       MeleeAttackSphereCenter() const;
	virtual float               MeleeAttackSphereRadius() const { return 0.8f; }

	virtual size_t              MaxInventory() const { return 1; }
	virtual size_t              MaxSlots() const { return 1; }

	virtual bool                TakesBlocks() const { return false; }
	virtual size_t              GiveBlocks(item_t eBlock, size_t iNumber, CBaseEntity* pGiveTo);
	virtual size_t              TakeBlocks(item_t eBlock, size_t iNumber);
	virtual size_t              RemoveBlocks(item_t eBlock, size_t iNumber);
	virtual bool                IsHoldingABlock() const;

protected:
	double                      m_flNextPlanetCheck;

	mutable CNetworkedHandle<CSpire>    m_hNearestSpire;
	mutable double                      m_flNextSpireCheck;

	double                      m_flLastEnteredAtmosphere;
	float						m_flRollFromSpace;

	size_t                      m_aiInventorySlots[INVENTORY_SLOTS];
	item_t                      m_aiInventoryTypes[INVENTORY_SLOTS];

	// AI stuffs
	task_t                      m_eTask;
	CEntityHandle<CStructure>   m_hBuild;
	IVector                     m_vecBuildDesignation;
	CEntityHandle<CStructure>   m_hMine;
	CEntityHandle<CStructure>   m_hWaitingForMine;
	CEntityHandle<CBaseEntity>  m_hEnemy;
};

#endif
