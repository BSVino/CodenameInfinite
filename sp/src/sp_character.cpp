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

void CSPCharacter::LockViewToPlanet()
{
	// Now lock the roll value to the planet.
	CPlanet* pNearestPlanet = GetNearestPlanet();
	if (!pNearestPlanet)
		return;

	Matrix4x4 mRotation = GetTransformation();
	mRotation.SetTranslation(Vector(0,0,0));

	// Construct a "local space" for the planet
	Vector vecPlanetUp = GetUpVector();
	Vector vecPlanetForward = mRotation.GetColumn(0);
	Vector vecPlanetRight = vecPlanetForward.Cross(vecPlanetUp).Normalized();
	vecPlanetForward = vecPlanetUp.Cross(vecPlanetRight).Normalized();

	Matrix4x4 mPlanet(vecPlanetForward, vecPlanetUp, vecPlanetRight);
	Matrix4x4 mPlanetInverse = mPlanet;
	mPlanetInverse.InvertTR();

	// Bring our current view angles into that local space
	Matrix4x4 mLocalRotation = mPlanetInverse * mRotation;
	EAngle angLocalRotation = mLocalRotation.GetAngles();

	// Lock them so that the roll is 0
	// I'm sure there's a way to do this without converting to euler but at this point I don't care.
	if (fabs(angLocalRotation.r) < 0.01f)
		return;

	angLocalRotation.r = 0;
	Matrix4x4 mLockedLocalRotation;
	mLockedLocalRotation.SetRotation(angLocalRotation);

	// Bring it back out to global space
	Matrix4x4 mLockedRotation = mPlanet * mLockedLocalRotation;

	// Only use the changed r value to avoid floating point crap
	EAngle angLockedRotation = GetAngles();
	angLockedRotation.r = mLockedRotation.GetAngles().r;
	SetAngles(angLockedRotation);
}

float CSPCharacter::CharacterSpeed()
{
	return 80000;
}
