#include "playerstart.h"

REGISTER_ENTITY(CPlayerStart);

NETVAR_TABLE_BEGIN(CPlayerStart);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN_EDITOR(CPlayerStart);
	SAVEDATA_OVERRIDE_DEFAULT(CSaveData::DATA_COPYTYPE, AABB, m_aabbBoundingBox, "BoundingBox", AABB(Vector(-0.35f, 0, -0.35f), Vector(0.35f, 2, 0.35f)));
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CPlayerStart);
INPUTS_TABLE_END();
