#include "sp_character.h"

#include <tinker/application.h>

#include "planet.h"

REGISTER_ENTITY(CSPCharacter);

NETVAR_TABLE_BEGIN(CSPCharacter);
	NETVAR_DEFINE(CEntityHandle, m_hNearestPlanet);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CSPCharacter);
	SAVEDATA_DEFINE(CSaveData::DATA_NETVAR, CEntityHandle<CPlanet>, m_hNearestPlanet);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, float, m_flNextPlanetCheck);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, float, m_flLastEnteredAtmosphere);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, float, m_flRollFromSpace);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CSPCharacter);
INPUTS_TABLE_END();

CSPCharacter::CSPCharacter()
{
	m_flNextPlanetCheck = 0;
	m_flLastEnteredAtmosphere = -1000;
}

void CSPCharacter::Think()
{
	BaseClass::Think();

	CPlanet* pPlanet = GetNearestPlanet();

	if (pPlanet && !HasMoveParent())
	{
		m_flLastEnteredAtmosphere = GameServer()->GetGameTime();
		m_flRollFromSpace = GetGlobalAngles().r;
	}

	SetMoveParent(pPlanet);
}

CPlanet* CSPCharacter::GetNearestPlanet(findplanet_t eFindPlanet)
{
	if (GameServer()->GetGameTime() > m_flNextPlanetCheck)
	{
		CPlanet* pNearestPlanet = FindNearestPlanet();

		float flDistance = CScalableFloat(pNearestPlanet->Distance(CScalableVector(GetGlobalOrigin(), SCALE_METER).GetUnits(SCALE_MEGAMETER)) - pNearestPlanet->GetRadius().GetUnits(SCALE_MEGAMETER), pNearestPlanet->GetScale()).GetUnits(SCALE_METER);
		if (flDistance < pNearestPlanet->GetAtmosphereThickness().GetUnits(SCALE_METER))
			m_hNearestPlanet = pNearestPlanet;
		else
			m_hNearestPlanet = NULL;

		m_flNextPlanetCheck = GameServer()->GetGameTime() + 0.3f;
	}

	if (eFindPlanet != FINDPLANET_INATMOSPHERE)
	{
		if (m_hNearestPlanet != NULL)
			return m_hNearestPlanet;

		CPlanet* pNearestPlanet = FindNearestPlanet();
		if (eFindPlanet == FINDPLANET_ANY)
			return pNearestPlanet;

		float flDistance = CScalableFloat(pNearestPlanet->Distance(CScalableVector(GetGlobalOrigin(), SCALE_METER).GetUnits(SCALE_MEGAMETER)) - pNearestPlanet->GetRadius().GetUnits(SCALE_MEGAMETER), pNearestPlanet->GetScale()).GetUnits(SCALE_METER);
		if (eFindPlanet == FINDPLANET_CLOSEORBIT && flDistance > pNearestPlanet->GetCloseOrbit().GetUnits(SCALE_METER))
			return NULL;
		else
			return pNearestPlanet;
	}

	return m_hNearestPlanet;
}

CPlanet* CSPCharacter::FindNearestPlanet()
{
	CPlanet* pNearestPlanet = NULL;
	float flNearestDistance;

	for (size_t i = 0; i < GameServer()->GetMaxEntities(); i++)
	{
		CPlanet* pPlanet = dynamic_cast<CPlanet*>(CBaseEntity::GetEntity(i));
		if (!pPlanet)
			continue;

		float flDistance = CScalableFloat(pPlanet->Distance(CScalableVector(GetGlobalOrigin(), SCALE_METER).GetUnits(SCALE_MEGAMETER)), pPlanet->GetScale()).GetUnits(SCALE_METER);
		flDistance -= pPlanet->GetRadius().GetUnits(SCALE_METER);

		if (pNearestPlanet == NULL)
		{
			pNearestPlanet = pPlanet;
			flNearestDistance = flDistance;
			continue;
		}

		if (flDistance > flNearestDistance)
			continue;

		pNearestPlanet = pPlanet;
		flNearestDistance = flDistance;
	}

	return pNearestPlanet;
}

Vector CSPCharacter::GetUpVector()
{
	CPlanet* pNearestPlanet = GetNearestPlanet();
	if (pNearestPlanet)
		return (GetGlobalOrigin() - pNearestPlanet->GetGlobalOrigin()).Normalized();

	return Vector(0, 1, 0);
}

void CSPCharacter::LockViewToPlanet()
{
	// Now lock the roll value to the planet.
	CPlanet* pNearestPlanet = GetNearestPlanet();
	if (!pNearestPlanet)
		return;

	Matrix4x4 mRotation = GetGlobalTransform();
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
	EAngle angNewLockedRotation = GetGlobalAngles();
	EAngle angOverloadRotation = mLockedRotation.GetAngles();

	// Lerp our way there
	float flTimeToLocked = 1;
	if (GameServer()->GetGameTime() - m_flLastEnteredAtmosphere > flTimeToLocked)
		angNewLockedRotation.r = angOverloadRotation.r;
	else
		angNewLockedRotation.r = RemapValClamped(SLerp(GameServer()->GetGameTime() - m_flLastEnteredAtmosphere, 0.3f), 0, flTimeToLocked, m_flRollFromSpace, angOverloadRotation.r);

	SetGlobalAngles(angNewLockedRotation);
}

void CSPCharacter::StandOnNearestPlanet()
{
	CPlanet* pPlanet = GetNearestPlanet(FINDPLANET_ANY);
	if (!pPlanet)
		return;

	CScalableVector vecPlanetOrigin(pPlanet->GetGlobalOrigin(), pPlanet->GetScale());
	CScalableVector vecCharacterOrigin(GetGlobalOrigin(), SCALE_METER);
	CScalableVector vecCharacterDirection = (vecCharacterOrigin - vecPlanetOrigin).Normalized();

	CScalableVector vecOrigin = vecPlanetOrigin + vecCharacterDirection * pPlanet->GetRadius();
	SetGlobalOrigin(vecOrigin.GetUnits(SCALE_METER));

	SetMoveParent(pPlanet);
}

float CSPCharacter::EyeHeight()
{
	// 180 centimeters
	return 0.18f;
}

float CSPCharacter::CharacterSpeed()
{
	CPlanet* pPlanet = GetNearestPlanet(FINDPLANET_CLOSEORBIT);

	float flSpeed = 80000000;

	if (pPlanet)
	{
		CScalableVector vecOrigin(GetGlobalOrigin(), SCALE_METER);
		CScalableFloat flDistance(pPlanet->Distance(vecOrigin.GetUnits(pPlanet->GetScale())), pPlanet->GetScale());
		flSpeed = RemapValClamped(flDistance.GetUnits(SCALE_METER), (pPlanet->GetRadius()+pPlanet->GetAtmosphereThickness()).GetUnits(SCALE_METER), (pPlanet->GetRadius()+pPlanet->GetCloseOrbit()).GetUnits(SCALE_METER), 20000.0f, 80000000);
	}

	return flSpeed;
}
