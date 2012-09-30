#pragma once

#include <game/entities/baseentity.h>

typedef enum
{
	STRUCTURE_NONE = 0,
	STRUCTURE_SPIRE,
	STRUCTURE_TOTAL,
} structure_type;

class CStructure : public CBaseEntity
{
	REGISTER_ENTITY_CLASS(CStructure, CBaseEntity);

public:
	void                    SetOwner(class CSPPlayer* pOwner) { m_hOwner = pOwner; }
	CSPPlayer*              GetOwner() const { return m_hOwner; }

	virtual void            PostConstruction() {};

public:
	static CStructure*      CreateStructure(structure_type eType, class CSPPlayer* pOwner, const CScalableVector& vecOrigin);

private:
	CEntityHandle<CSPPlayer>  m_hOwner;
};
