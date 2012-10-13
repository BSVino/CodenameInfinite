#pragma once

#include <game/entities/baseentity.h>

typedef enum
{
	STRUCTURE_NONE = 0,
	STRUCTURE_SPIRE,
	STRUCTURE_MINE,
	STRUCTURE_TOTAL,
} structure_type;

class CSpire;

class CStructure : public CBaseEntity
{
	REGISTER_ENTITY_CLASS(CStructure, CBaseEntity);

public:
	void                    SetOwner(class CSPPlayer* pOwner);
	CSPPlayer*              GetOwner() const;

	void                    SetSpire(CSpire* pSpire);
	CSpire*                 GetSpire() const;

	virtual void            PostConstruction() {};

	virtual void            PerformStructureTask() {};

	virtual const Matrix4x4 GetPhysicsTransform() const;

public:
	static CStructure*      CreateStructure(structure_type eType, class CSPPlayer* pOwner, CSpire* pSpire, const CScalableVector& vecOrigin);

private:
	CEntityHandle<CSPPlayer>  m_hOwner;
	CEntityHandle<CSpire>     m_hSpire;
};
