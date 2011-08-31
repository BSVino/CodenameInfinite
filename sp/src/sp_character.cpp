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

	if (pPlanet && !HasScalableMoveParent())
	{
		m_flLastEnteredAtmosphere = GameServer()->GetGameTime();
		m_flRollFromSpace = GetGlobalScalableAngles().r;
		SetScalableMoveParent(pPlanet);
	}

	if (pPlanet)
	{
		// Estimate this planet's mass given things we know about earth. Assume equal densities.
		double flEarthVolume = 1097509500000000000000.0;	// cubic meters
		double flEarthMass = 5974200000000000000000000.0;	// kilograms

		// 4/3 * pi * r^3 = volume of a sphere
		CScalableFloat flPlanetVolume = pPlanet->GetRadius()*pPlanet->GetRadius()*pPlanet->GetRadius()*(M_PI*4/3);
		double flPlanetMass = RemapVal(flPlanetVolume, CScalableFloat(), CScalableFloat(flEarthVolume, SCALE_METER), 0, flEarthMass);

		double flG = 0.0000000000667384;					// Gravitational constant

		CScalableVector vecDistance = (pPlanet->GetGlobalScalableOrigin() - GetGlobalScalableOrigin());
		CScalableFloat flDistance = vecDistance.Length();
		CScalableFloat flGravity = CScalableFloat(flPlanetMass*flG, SCALE_METER)/(flDistance*flDistance);

		CScalableVector vecGravity = vecDistance * flGravity / flDistance;

		SetGlobalScalableGravity(vecGravity);
	}
}

void CSPCharacter::MoveThink()
{
	if (!GetGroundEntity())
		return;

	CSPEntity* pSPGroundEntity = dynamic_cast<CSPEntity*>(GetGroundEntity());
	if (!pSPGroundEntity)
		return;

	if (m_vecGoalVelocity.LengthSqr())
		m_vecGoalVelocity.Normalize();

	m_vecMoveVelocity.x = Approach(m_vecGoalVelocity.x, m_vecMoveVelocity.x, GameServer()->GetFrameTime()*4);
	m_vecMoveVelocity.y = Approach(m_vecGoalVelocity.y, m_vecMoveVelocity.y, GameServer()->GetFrameTime()*4);
	m_vecMoveVelocity.z = Approach(m_vecGoalVelocity.z, m_vecMoveVelocity.z, GameServer()->GetFrameTime()*4);

	Vector vecUp = (GetGlobalScalableOrigin() - pSPGroundEntity->GetGlobalScalableOrigin()).NormalizedVector();
	Vector vecRight = vecUp.Cross(m_vecMoveVelocity);
	Vector vecForward = vecRight.Cross(vecUp);

	if (m_vecMoveVelocity.LengthSqr() > 0)
	{
		CScalableVector vecVelocity = GetLocalScalableVelocity();

		CScalableMatrix m = GetLocalScalableTransform();
		m.SetTranslation(CScalableVector());

		CScalableVector vecMove = vecForward * (CharacterSpeedScalable() * GameServer()->GetFrameTime());
		vecVelocity = m * vecMove;

		SetLocalScalableVelocity(vecVelocity);
	}
	else
		SetLocalScalableVelocity(CScalableVector());
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

	CScalableVector vecOrigin = vecPlanetOrigin + vecCharacterDirection * pPlanet->GetRadius();
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
	return CScalableFloat(2.0f, SCALE_METER);
}

float CSPCharacter::EyeHeight()
{
	TAssert(false);
	// 180 centimeters
	return 0.18f;
}

float CSPCharacter::CharacterSpeed()
{
	TAssert(false);
	return 80000;
}

bool CSPCharacter::ShouldTouch(ISPEntity* pOther)
{
	return true;
}

CScalableVector CSPCharacter::GetGlobalScalableGravity() const
{
	if (GetGroundEntity())
		return CScalableVector();

	return ISPEntity::GetGlobalScalableGravity();
}

void CSPCharacter::FindGroundEntity()
{
	CScalableVector vecUp = GetGlobalScalableTransform().GetUpVector() * CScalableFloat(1.0f, SCALE_METER);

	size_t iMaxEntities = GameServer()->GetMaxEntities();
	for (size_t j = 0; j < iMaxEntities; j++)
	{
		CBaseEntity* pEntity = CBaseEntity::GetEntity(j);

		if (!pEntity)
			continue;

		if (pEntity->IsDeleted())
			continue;

		ISPEntity* pSPEntity = dynamic_cast<ISPEntity*>(pEntity);
		if (!pSPEntity)
			continue;

		if (pSPEntity == this)
			continue;

		if (!ShouldTouch(pSPEntity))
			continue;

		CScalableVector vecPoint;
		if (ISPEntity::IsTouching(pSPEntity, GetGlobalScalableOrigin() - vecUp, vecPoint))
		{
			SetGroundEntity(pEntity);
			SetSimulated(false);
			return;
		}
	}

	SetGroundEntity(NULL);
	SetSimulated(true);
}
