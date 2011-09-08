#include "sp_playercharacter.h"

#include <tinker/application.h>

#include "planet.h"

REGISTER_ENTITY(CPlayerCharacter);

NETVAR_TABLE_BEGIN(CPlayerCharacter);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CPlayerCharacter);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, bool, m_bFlying);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, bool, m_bWalkSpeedOverride);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, bool, m_bHyperdrive);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CPlayerCharacter);
INPUTS_TABLE_END();

CPlayerCharacter::CPlayerCharacter()
{
	m_bHyperdrive = false;
	m_bWalkSpeedOverride = false;
	m_bFlying = false;
}

void CPlayerCharacter::Think()
{
	BaseClass::Think();
}

void CPlayerCharacter::MoveThink()
{
	if (!m_bFlying)
	{
		BaseClass::MoveThink();
		return;
	}

	if (m_vecGoalVelocity.LengthSqr())
		m_vecGoalVelocity.Normalize();

	m_vecMoveVelocity.x = Approach(m_vecGoalVelocity.x, m_vecMoveVelocity.x, GameServer()->GetFrameTime()*4);
	m_vecMoveVelocity.y = Approach(m_vecGoalVelocity.y, m_vecMoveVelocity.y, GameServer()->GetFrameTime()*4);
	m_vecMoveVelocity.z = Approach(m_vecGoalVelocity.z, m_vecMoveVelocity.z, GameServer()->GetFrameTime()*4);

#ifndef _DEBUG
	if (m_bHyperdrive)
		m_vecMoveVelocity.y = m_vecMoveVelocity.z = 0;
#endif

	if (m_vecMoveVelocity.LengthSqr() > 0)
	{
		CScalableVector vecVelocity = GetLocalVelocity();

		CScalableMatrix m = GetLocalTransform();
		m.SetTranslation(CScalableVector());

		CScalableVector vecMove = m_vecMoveVelocity * CharacterSpeed();
		vecVelocity = m * vecMove;

		SetLocalVelocity(vecVelocity);
	}
	else
		SetLocalVelocity(CScalableVector());
}

void CPlayerCharacter::ToggleFlying()
{
	if (m_bFlying)
		StopFlying();
	else
		StartFlying();
}

void CPlayerCharacter::StartFlying()
{
	if (m_bFlying)
		return;

	m_bFlying = true;
	SetGroundEntity(NULL);
	SetSimulated(true);
}

void CPlayerCharacter::StopFlying()
{
	m_bFlying = false;
	SetSimulated(false);
}

CScalableFloat CPlayerCharacter::CharacterSpeed()
{
	if (!m_bFlying || m_bWalkSpeedOverride)
		return BaseClass::CharacterSpeed();

	float flDebugBonus = 1;
#ifdef _DEBUG
	if (m_bHyperdrive)
		flDebugBonus = 10;
#endif

	CPlanet* pPlanet = GetNearestPlanet(FINDPLANET_ANY);

	CScalableFloat flMaxSpeed = CScalableFloat(1.0f, SCALE_MEGAMETER);

	CScalableFloat flDistance = (pPlanet->GetGlobalOrigin() - GetGlobalOrigin()).Length();
	CScalableFloat flCloseOrbit = pPlanet->GetRadius()+pPlanet->GetCloseOrbit();

	if (flDistance < flCloseOrbit)
	{
		CScalableFloat flAtmosphereSpeed = CScalableFloat(50.0f, SCALE_KILOMETER);

		CScalableFloat flAtmosphere = pPlanet->GetRadius()+pPlanet->GetAtmosphereThickness();

		if (flDistance < flAtmosphere)
		{
			CScalableFloat flGroundSpeed = CScalableFloat(0.5f, SCALE_KILOMETER);
			return RemapVal(flDistance, pPlanet->GetRadius(), flAtmosphere, flGroundSpeed, flAtmosphereSpeed) * flDebugBonus;
		}

		if (flDistance > flCloseOrbit)
			return flMaxSpeed * flDebugBonus;

		return RemapVal(flDistance, flAtmosphere, flCloseOrbit, flAtmosphereSpeed, flMaxSpeed) * flDebugBonus;
	}

	if (m_bHyperdrive)
	{
		CScalableFloat flMinSpeed = flMaxSpeed;
		flMaxSpeed = CScalableFloat(1.0f, SCALE_GIGAMETER);
		return RemapVal(flDistance, flCloseOrbit, CScalableFloat(0.5f, SCALE_GIGAMETER), flMinSpeed, flMaxSpeed);
	}
	else
		return flMaxSpeed;
}

CScalableVector CPlayerCharacter::GetGlobalGravity() const
{
	if (m_bFlying)
		return CScalableVector();

	return BaseClass::GetGlobalGravity();
}

void CPlayerCharacter::FindGroundEntity()
{
	if (m_bFlying)
	{
		SetGroundEntity(NULL);
		return;
	}

	BaseClass::FindGroundEntity();
}
