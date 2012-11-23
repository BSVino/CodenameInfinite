#pragma once

#include <game/entities/baseentity.h>

#include "entities/items/items.h"

typedef enum
{
	STRUCTURE_NONE = 0,
	STRUCTURE_SPIRE,
	STRUCTURE_MINE,
	STRUCTURE_PALLET,
	STRUCTURE_STOVE,
	STRUCTURE_TOTAL,
} structure_type;

class CSpire;

class CStructure : public CBaseEntity
{
	REGISTER_ENTITY_CLASS(CStructure, CBaseEntity);

public:
	void                    Spawn();

	void                    SetOwner(class CSPPlayer* pOwner);
	CSPPlayer*              GetOwner() const;

	void                    SetSpire(CSpire* pSpire);
	CSpire*                 GetSpire() const;

	void                    Think();

	void                    PostRender() const;

	bool                    CanAutoOpenMenu() const;
	bool                    CanAutoCloseMenu() const;

	void                    SetTurnsToConstruct(int iTurns);
	bool                    IsUnderConstruction() const { return m_iTurnsToConstruct > 0; }
	bool                    IsWorkingConstructionTurn() const { return !!m_flConstructionTurnTime; }
	int                     GetTurnsToConstruct() const { return m_iTurnsToConstruct; }
	int                     GetTotalTurnsToConstruct() const { return m_iTotalTurnsToConstruct; }
	void                    ConstructionTurn();
	void                    FinishConstruction();

	virtual void            PostConstruction() {};
	virtual void            PostConstructionFinished() {};

	virtual void            OnUse(CBaseEntity* pUser);
	virtual void            SetupMenuButtons();
	virtual void            PerformStructureTask(class CSPCharacter* pCharacter);
	virtual bool            IsOccupied() const;

	virtual const Matrix4x4 GetPhysicsTransform() const;
	virtual void            SetPhysicsTransform(const Matrix4x4& m);
	virtual void            PostSetLocalTransform(const TMatrix& m);
	collision_group_t       GetCollisionGroup() const { return CG_STATIC; }

	virtual bool            TakesBlocks() const { return false; }
	virtual size_t          TakeBlocks(item_t eBlock, size_t iNumber) { return 0; }

	virtual void            MenuCommand(const tstring& sCommand) {};

	virtual structure_type  StructureType() const { return STRUCTURE_NONE; }

public:
	static CStructure*      CreateStructure(structure_type eType, class CSPPlayer* pOwner, CSpire* pSpire, const CScalableVector& vecOrigin);

private:
	CEntityHandle<CSPPlayer>  m_hOwner;
	CEntityHandle<CSpire>     m_hSpire;
	structure_type            m_eStructureType;

	int             m_iTurnsToConstruct;
	int             m_iTotalTurnsToConstruct;
	double          m_flConstructionTurnTime;
};

const char* GetStructureName(structure_type eStructure);
