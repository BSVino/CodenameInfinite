#include "sp_character.h"

#include "planet.h"

REGISTER_ENTITY(CSPCharacter);

NETVAR_TABLE_BEGIN(CSPCharacter);
	NETVAR_DEFINE(CEntityHandle, m_hNearestPlanet);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CSPCharacter);
	SAVEDATA_DEFINE(CSaveData::DATA_NETVAR, CEntityHandle<CPlanet>, m_hNearestPlanet);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CSPCharacter);
INPUTS_TABLE_END();

CPlanet* CSPCharacter::GetNearestPlanet()
{
	if (m_hNearestPlanet == NULL)
	{
		float flNearestDistance;
		for (size_t i = 0; i < GameServer()->GetMaxEntities(); i++)
		{
			CPlanet* pPlanet = dynamic_cast<CPlanet*>(CBaseEntity::GetEntity(i));
			if (!pPlanet)
				continue;

			float flDistance = pPlanet->Distance(GetOrigin());
			if (flDistance > 10000)
				continue;

			if (m_hNearestPlanet == NULL)
			{
				m_hNearestPlanet = pPlanet;
				flNearestDistance = flDistance;
				continue;
			}

			if (flDistance > flNearestDistance)
				continue;

			m_hNearestPlanet = pPlanet;
			flNearestDistance = flDistance;
		}
	}

	return m_hNearestPlanet;
}

Vector CSPCharacter::GetUpVector()
{
	CPlanet* pNearestPlanet = GetNearestPlanet();
	if (pNearestPlanet)
		return (GetOrigin() - pNearestPlanet->GetOrigin()).Normalized();

	return Vector(0, 1, 0);
}
