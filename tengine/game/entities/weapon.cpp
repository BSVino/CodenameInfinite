#include "weapon.h"

REGISTER_ENTITY(CBaseWeapon);

NETVAR_TABLE_BEGIN(CBaseWeapon);
	NETVAR_DEFINE(CEntityHandle, m_hHolder);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CBaseWeapon);
	SAVEDATA_DEFINE(CSaveData::DATA_NETVAR, CEntityHandle<CBaseEntity>, m_hHolder);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CBaseWeapon);
INPUTS_TABLE_END();
