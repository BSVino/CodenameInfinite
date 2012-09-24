#include "sp_character.h"

#include <tinker/application.h>

#include "planet.h"
#include "sp_game.h"
#include "sp_renderer.h"
#include "sp_playercharacter.h"

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

void CSPCharacter::Spawn()
{
	BaseClass::Spawn();

	m_flMaxStepSize = TFloat(0.1f, SCALE_METER);
}

void CSPCharacter::Think()
{
	BaseClass::Think();

	CPlanet* pPlanet = GetNearestPlanet();

	if (pPlanet && !IsInPhysics())
		AddToPhysics(CT_CHARACTER);
	else if (!pPlanet && IsInPhysics())
		RemoveFromPhysics();

	bool bHadParent = HasMoveParent();
	if (pPlanet && !HasMoveParent())
	{
		m_flLastEnteredAtmosphere = GameServer()->GetGameTime();
		m_flRollFromSpace = GetViewAngles().r;
	}

	SetMoveParent(pPlanet);

	if (pPlanet && !bHadParent)
		EnteredAtmosphere();

	if (pPlanet && IsInPhysics())
		GamePhysics()->SetEntityUpVector(this, GetUpVector());

	if (pPlanet)
	{
		// Estimate this planet's mass given things we know about earth. Assume equal densities.
		double flEarthVolume = 1097509500000000000000.0;	// cubic meters
		double flEarthMass = 5974200000000000000000000.0;	// kilograms

		// 4/3 * pi * r^3 = volume of a sphere
		CScalableFloat flPlanetVolume = pPlanet->GetRadius()*pPlanet->GetRadius()*pPlanet->GetRadius()*(M_PI*4/3);
		double flPlanetMass = RemapVal(flPlanetVolume, CScalableFloat(), CScalableFloat(flEarthVolume, SCALE_METER), 0, flEarthMass);

		double flG = 0.0000000000667384;					// Gravitational constant

		CScalableVector vecDistance = (pPlanet->GetGlobalOrigin() - GetGlobalOrigin());
		CScalableFloat flDistance = vecDistance.Length();
		CScalableFloat flGravity = CScalableFloat(flPlanetMass*flG, SCALE_METER)/(flDistance*flDistance);

		CScalableVector vecGravity = vecDistance * flGravity / flDistance;

		SetGlobalGravity(vecGravity);
	}
}

const CScalableMatrix CSPCharacter::GetScalableRenderTransform() const
{
	CPlayerCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
	CScalableVector vecCharacterOrigin = pCharacter->GetGlobalOrigin();

	CScalableMatrix mTransform = GetGlobalTransform();
	mTransform.SetTranslation(mTransform.GetTranslation() - vecCharacterOrigin);

	return mTransform;
}

const CScalableVector CSPCharacter::GetScalableRenderOrigin() const
{
	return GetScalableRenderTransform().GetTranslation();
}

const Matrix4x4 CSPCharacter::GetRenderTransform() const
{
	scale_t eScale = SPGame()->GetSPRenderer()->GetRenderingScale();
	TAssert(eScale != SCALE_NONE);
	return GetScalableRenderTransform().GetUnits(eScale);
}

const Vector CSPCharacter::GetRenderOrigin() const
{
	scale_t eScale = SPGame()->GetSPRenderer()->GetRenderingScale();
	TAssert(eScale != SCALE_NONE);
	return GetScalableRenderTransform().GetTranslation().GetUnits(eScale);
}

CPlanet* CSPCharacter::GetNearestPlanet(findplanet_t eFindPlanet)
{
	if (GameServer()->GetGameTime() > m_flNextPlanetCheck)
	{
		CPlanet* pNearestPlanet = FindNearestPlanet();

		CScalableFloat flDistance = (pNearestPlanet->GetGlobalOrigin() - GetGlobalOrigin()).Length() - pNearestPlanet->GetRadius();
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

		CScalableFloat flDistance = (pNearestPlanet->GetGlobalOrigin() - GetGlobalOrigin()).Length() - pNearestPlanet->GetRadius();
		if (eFindPlanet == FINDPLANET_CLOSEORBIT && flDistance > pNearestPlanet->GetCloseOrbit())
			return NULL;
		else
			return pNearestPlanet;
	}

	return m_hNearestPlanet;
}

CPlanet* CSPCharacter::GetNearestPlanet(findplanet_t eFindPlanet) const
{
	if (eFindPlanet != FINDPLANET_INATMOSPHERE)
	{
		if (m_hNearestPlanet != NULL)
			return m_hNearestPlanet;

		CPlanet* pNearestPlanet = FindNearestPlanet();
		if (eFindPlanet == FINDPLANET_ANY)
			return pNearestPlanet;

		CScalableFloat flDistance = (pNearestPlanet->GetGlobalOrigin() - GetGlobalOrigin()).Length() - pNearestPlanet->GetRadius();
		if (eFindPlanet == FINDPLANET_CLOSEORBIT && flDistance > pNearestPlanet->GetCloseOrbit())
			return NULL;
		else
			return pNearestPlanet;
	}

	return m_hNearestPlanet;
}

CPlanet* CSPCharacter::FindNearestPlanet() const
{
	CPlanet* pNearestPlanet = NULL;
	CScalableFloat flNearestDistance;

	for (size_t i = 0; i < GameServer()->GetMaxEntities(); i++)
	{
		CPlanet* pPlanet = dynamic_cast<CPlanet*>(CBaseEntity::GetEntity(i));
		if (!pPlanet)
			continue;

		CScalableFloat flDistance = (pPlanet->GetGlobalOrigin() - GetGlobalOrigin()).Length();
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

const Vector CSPCharacter::GetUpVector() const
{
	CPlanet* pNearestPlanet = GetNearestPlanet();
	if (pNearestPlanet)
		return Vector(GetGlobalOrigin() - pNearestPlanet->GetGlobalOrigin()).Normalized();

	return Vector(0, 1, 0);
}

const Vector CSPCharacter::GetLocalUpVector() const
{
	CPlanet* pNearestPlanet = GetNearestPlanet();
	if (pNearestPlanet)
		return Vector(GetLocalOrigin()).Normalized();

	return Vector(0, 1, 0);
}

void CSPCharacter::LockViewToPlanet()
{
	// Now lock the roll value to the planet.
	CPlanet* pNearestPlanet = GetNearestPlanet();
	if (!pNearestPlanet)
		return;

	Matrix4x4 mPlanetLocalRotation(GetViewAngles());

	// Construct a "local space" for the planet
	Vector vecSurfaceUp = GetLocalUpVector();
	Vector vecSurfaceForward = mPlanetLocalRotation.GetForwardVector();
	Vector vecSurfaceRight = vecSurfaceForward.Cross(vecSurfaceUp).Normalized();
	vecSurfaceForward = vecSurfaceUp.Cross(vecSurfaceRight).Normalized();

	Matrix4x4 mSurface(vecSurfaceForward, vecSurfaceUp, vecSurfaceRight);
	Matrix4x4 mSurfaceInverse = mSurface;
	mSurfaceInverse.InvertRT();

	// Bring our current view angles into that local space
	Matrix4x4 mLocalRotation = mSurfaceInverse * mPlanetLocalRotation;
	EAngle angLocalRotation = mLocalRotation.GetAngles();

	// Lock them so that the roll is 0
	// I'm sure there's a way to do this without converting to euler but at this point I don't care.
	angLocalRotation.r = 0;
	Matrix4x4 mLockedLocalRotation;
	mLockedLocalRotation.SetAngles(angLocalRotation);

	// Bring it back out to global space
	Matrix4x4 mLockedRotation = mSurface * mLockedLocalRotation;

	// Only use the changed r value to avoid floating point crap
	EAngle angNewLockedRotation = GetViewAngles();
	EAngle angOverloadRotation = mLockedRotation.GetAngles();

	// Lerp our way there
	float flTimeToLocked = GetAtmosphereLerpTime();
	if (GameServer()->GetGameTime() - m_flLastEnteredAtmosphere > flTimeToLocked)
		angNewLockedRotation.r = angOverloadRotation.r;
	else
	{
		float flRollDifference = AngleDifference(angOverloadRotation.r, m_flRollFromSpace);
		angNewLockedRotation.r = m_flRollFromSpace + RemapValClamped(SLerp((float)(GameServer()->GetGameTime() - m_flLastEnteredAtmosphere), GetAtmosphereLerp()), 0, flTimeToLocked, 0, flRollDifference);
	}

	SetViewAngles(angNewLockedRotation);
}

void CSPCharacter::StandOnNearestPlanet()
{
	CPlanet* pPlanet = GetNearestPlanet(FINDPLANET_ANY);
	if (!pPlanet)
		return;

	SetMoveParent(pPlanet);

	CScalableVector vecPlanetOrigin = pPlanet->GetLocalOrigin();
	CScalableVector vecCharacterOrigin = GetLocalOrigin();
	CScalableVector vecCharacterDirection = (vecCharacterOrigin - vecPlanetOrigin).Normalized();

	TUnimplemented();

	/*
	CTraceResult tr;
	pPlanet->CollideLocalAccurate(this, true, vecCharacterDirection * (pPlanet->GetRadius()*2.0f), TVector(), tr);

	TFloat flPlanetRadius = pPlanet->GetRadius();
	TVector vecPoint = (vecCharacterOrigin - vecPlanetOrigin).Normalized() * flPlanetRadius;

	TAssert(tr.bHit);

	SetLocalOrigin(tr.vecHit);
	*/
}

CScalableFloat CSPCharacter::EyeHeight() const
{
	// 180 centimeters
	return CScalableFloat(1.8f, SCALE_METER);
}

CScalableFloat CSPCharacter::JumpStrength()
{
	return CScalableFloat(3.0f, SCALE_METER);
}

CScalableFloat CSPCharacter::CharacterSpeed()
{
	return CScalableFloat(5.0f, SCALE_METER);
}
