#include "sp_character.h"

#include <tinker/application.h>

#include "planet.h"

REGISTER_ENTITY(CSPCharacter);

NETVAR_TABLE_BEGIN(CSPCharacter);
	NETVAR_DEFINE(CEntityHandle, m_hNearestPlanet);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CSPCharacter);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CEntityHandle<CBaseEntity>, m_hScalableMoveParent);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYVECTOR, CEntityHandle<CBaseEntity>, m_ahScalableMoveChildren);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, bool, m_bGlobalScalableTransformsDirty);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CScalableMatrix, m_mGlobalScalableTransform);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CScalableVector, m_vecGlobalScalableGravity);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CScalableMatrix, m_mLocalScalableTransform);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CScalableVector, m_vecLocalScalableOrigin);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CScalableVector, m_vecLastLocalScalableOrigin);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, EAngle, m_angLocalScalableAngles);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CScalableVector, m_vecLocalScalableVelocity);

	SAVEDATA_DEFINE(CSaveData::DATA_NETVAR, CEntityHandle<CPlanet>, m_hNearestPlanet);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, float, m_flNextPlanetCheck);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, float, m_flLastEnteredAtmosphere);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, float, m_flRollFromSpace);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, bool, m_bHyperdrive);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CSPCharacter);
INPUTS_TABLE_END();

CSPCharacter::CSPCharacter()
{
	m_flNextPlanetCheck = 0;
	m_flLastEnteredAtmosphere = -1000;
	m_bHyperdrive = false;
}

void CSPCharacter::Think()
{
	if (m_vecGoalVelocity.LengthSqr())
		m_vecGoalVelocity.Normalize();

	m_vecMoveVelocity.x = Approach(m_vecGoalVelocity.x, m_vecMoveVelocity.x, GameServer()->GetFrameTime()*4);
	m_vecMoveVelocity.y = Approach(m_vecGoalVelocity.y, m_vecMoveVelocity.y, GameServer()->GetFrameTime()*4);
	m_vecMoveVelocity.z = Approach(m_vecGoalVelocity.z, m_vecMoveVelocity.z, GameServer()->GetFrameTime()*4);

	if (m_bHyperdrive)
		m_vecMoveVelocity.y = m_vecMoveVelocity.z = 0;

	if (m_vecMoveVelocity.LengthSqr() > 0)
	{
		CScalableVector vecVelocity = GetLocalScalableVelocity();

		CScalableMatrix m = GetLocalScalableTransform();
		m.SetTranslation(CScalableVector());

		CScalableVector vecMove = m_vecMoveVelocity * (CharacterSpeedScalable() * GameServer()->GetFrameTime());
		vecVelocity = m * vecMove;

		SetLocalScalableVelocity(vecVelocity);
	}
	else
		SetLocalScalableVelocity(CScalableVector());

	CPlanet* pPlanet = GetNearestPlanet();

	if (pPlanet && !HasScalableMoveParent())
	{
		m_flLastEnteredAtmosphere = GameServer()->GetGameTime();
		m_flRollFromSpace = GetGlobalScalableAngles().r;
	}

	SetScalableMoveParent(pPlanet);
}

CPlanet* CSPCharacter::GetNearestPlanet(findplanet_t eFindPlanet)
{
	if (GameServer()->GetGameTime() > m_flNextPlanetCheck)
	{
		CPlanet* pNearestPlanet = FindNearestPlanet();

		CScalableFloat flDistance = (pNearestPlanet->GetGlobalScalableOrigin() - GetGlobalScalableOrigin()).Length() - pNearestPlanet->GetRadius();
		if (flDistance < pNearestPlanet->GetAtmosphereThickness())
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

		CScalableFloat flDistance = (pNearestPlanet->GetGlobalScalableOrigin() - GetGlobalScalableOrigin()).Length() - pNearestPlanet->GetRadius();
		if (eFindPlanet == FINDPLANET_CLOSEORBIT && flDistance > pNearestPlanet->GetCloseOrbit())
			return NULL;
		else
			return pNearestPlanet;
	}

	return m_hNearestPlanet;
}

CPlanet* CSPCharacter::FindNearestPlanet()
{
	CPlanet* pNearestPlanet = NULL;
	CScalableFloat flNearestDistance;

	for (size_t i = 0; i < GameServer()->GetMaxEntities(); i++)
	{
		CPlanet* pPlanet = dynamic_cast<CPlanet*>(CBaseEntity::GetEntity(i));
		if (!pPlanet)
			continue;

		CScalableFloat flDistance = (pPlanet->GetGlobalScalableOrigin() - GetGlobalScalableOrigin()).Length();
		flDistance -= pPlanet->GetRadius();

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
		return (GetGlobalScalableOrigin() - pNearestPlanet->GetGlobalScalableOrigin()).Normalized().GetUnits(SCALE_METER);

	return Vector(0, 1, 0);
}

void CSPCharacter::LockViewToPlanet()
{
	// Now lock the roll value to the planet.
	CPlanet* pNearestPlanet = GetNearestPlanet();
	if (!pNearestPlanet)
		return;

	CScalableMatrix mRotation = GetGlobalScalableTransform();
	mRotation.SetTranslation(CScalableVector());

	// Construct a "local space" for the planet
	Vector vecPlanetUp = GetUpVector();
	Vector vecPlanetForward = mRotation.GetColumn(0);
	Vector vecPlanetRight = vecPlanetForward.Cross(vecPlanetUp).Normalized();
	vecPlanetForward = vecPlanetUp.Cross(vecPlanetRight).Normalized();

	CScalableMatrix mPlanet(vecPlanetForward, vecPlanetUp, vecPlanetRight);
	CScalableMatrix mPlanetInverse = mPlanet;
	mPlanetInverse.InvertTR();

	// Bring our current view angles into that local space
	CScalableMatrix mLocalRotation = mPlanetInverse * mRotation;
	EAngle angLocalRotation = mLocalRotation.GetAngles();

	// Lock them so that the roll is 0
	// I'm sure there's a way to do this without converting to euler but at this point I don't care.
	angLocalRotation.r = 0;
	CScalableMatrix mLockedLocalRotation;
	mLockedLocalRotation.SetAngles(angLocalRotation);

	// Bring it back out to global space
	CScalableMatrix mLockedRotation = mPlanet * mLockedLocalRotation;

	// Only use the changed r value to avoid floating point crap
	EAngle angNewLockedRotation = GetGlobalScalableAngles();
	EAngle angOverloadRotation = mLockedRotation.GetAngles();

	// Lerp our way there
	float flTimeToLocked = 1;
	if (GameServer()->GetGameTime() - m_flLastEnteredAtmosphere > flTimeToLocked)
		angNewLockedRotation.r = angOverloadRotation.r;
	else
		angNewLockedRotation.r = RemapValClamped(SLerp(GameServer()->GetGameTime() - m_flLastEnteredAtmosphere, 0.3f), 0, flTimeToLocked, m_flRollFromSpace, angOverloadRotation.r);

	SetGlobalScalableAngles(angNewLockedRotation);
}

void CSPCharacter::StandOnNearestPlanet()
{
	CPlanet* pPlanet = GetNearestPlanet(FINDPLANET_ANY);
	if (!pPlanet)
		return;

	CScalableVector vecPlanetOrigin = pPlanet->GetGlobalScalableOrigin();
	CScalableVector vecCharacterOrigin = GetGlobalScalableOrigin();
	CScalableVector vecCharacterDirection = (vecCharacterOrigin - vecPlanetOrigin).Normalized();

	// Right now it puts us one kilometer above ground. We'll fix that later.
	CScalableVector vecOrigin = vecPlanetOrigin + vecCharacterDirection * (pPlanet->GetRadius() + CScalableFloat(1, SCALE_KILOMETER));
	SetGlobalScalableOrigin(vecOrigin);

	SetScalableMoveParent(pPlanet);
}

CScalableFloat CSPCharacter::EyeHeightScalable()
{
	// 180 centimeters
	return CScalableFloat(0.18f, SCALE_METER);
}

CScalableFloat CSPCharacter::CharacterSpeedScalable()
{
	CPlanet* pPlanet = GetNearestPlanet(FINDPLANET_ANY);

	CScalableFloat flMaxSpeed = CScalableFloat(10, SCALE_MEGAMETER);

	CScalableFloat flDistance = (pPlanet->GetGlobalScalableOrigin() - GetGlobalScalableOrigin()).Length();
	CScalableFloat flCloseOrbit = pPlanet->GetRadius()+pPlanet->GetCloseOrbit();

	if (flDistance < flCloseOrbit)
	{
		CScalableFloat flAtmosphereSpeed = CScalableFloat(500, SCALE_KILOMETER);

		CScalableFloat flAtmosphere = pPlanet->GetRadius()+pPlanet->GetAtmosphereThickness();

		if (flDistance < flAtmosphere)
		{
			CScalableFloat flGroundSpeed = CScalableFloat(1, SCALE_KILOMETER);
			return RemapVal(flDistance, pPlanet->GetRadius(), flAtmosphere, flGroundSpeed, flAtmosphereSpeed);
		}

		if (flDistance > flCloseOrbit)
			return flMaxSpeed;

		return RemapVal(flDistance, flAtmosphere, flCloseOrbit, flAtmosphereSpeed, flMaxSpeed);
	}

	if (m_bHyperdrive)
	{
		CScalableFloat flMinSpeed = flMaxSpeed;
		flMaxSpeed = CScalableFloat(50, SCALE_GIGAMETER);
		return RemapVal(flDistance, flCloseOrbit, CScalableFloat(1, SCALE_GIGAMETER), flMinSpeed, flMaxSpeed);
	}
	else
		return flMaxSpeed;
}

float CSPCharacter::EyeHeight()
{
	// 180 centimeters
	return 0.18f;
}

float CSPCharacter::CharacterSpeed()
{
	CPlanet* pPlanet = GetNearestPlanet(FINDPLANET_CLOSEORBIT);

	float flSpeed = 80000;

	if (pPlanet)
	{
		CScalableVector vecOrigin = GetGlobalScalableOrigin();
		CScalableFloat flDistance = (pPlanet->GetGlobalScalableOrigin() - vecOrigin).Length();
		flSpeed = RemapValClamped(
			flDistance.GetUnits(SCALE_KILOMETER),
			(pPlanet->GetRadius()+pPlanet->GetAtmosphereThickness()).GetUnits(SCALE_KILOMETER),
			(pPlanet->GetRadius()+pPlanet->GetCloseOrbit()).GetUnits(SCALE_KILOMETER),
			200.0f, 80000);
	}

	return flSpeed;
}
