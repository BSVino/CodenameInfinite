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
	static CStructure*      CreateStructure(structure_type eType, class CSPPlayer* pOwner, const CScalableVector& vecOrigin);
};
