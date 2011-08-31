#include "sp_playercharacter.h"

#include <tinker/application.h>

#include "planet.h"

REGISTER_ENTITY(CPlayerCharacter);

NETVAR_TABLE_BEGIN(CPlayerCharacter);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CPlayerCharacter);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, bool, m_bFlying);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, bool, m_bHyperdrive);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CPlayerCharacter);
INPUTS_TABLE_END();

CPlayerCharacter::CPlayerCharacter()
{
	m_bHyperdrive = false;
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
		CScalableVector vecVelocity = GetLocalScalableVelocity();

		CScalableMatrix m = GetLocalScalableTransform();
		m.SetTranslation(CScalableVector());

		CScalableVector vecMove = m_vecMoveVelocity * (CharacterSpeedScalable() * GameServer()->GetFrameTime());
		vecVelocity = m * vecMove;

		SetLocalScalableVelocity(vecVelocity);
	}
	else
		SetLocalScalableVelocity(CScalableVector());
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
	SetSimulated(true);
}

CScalableFloat CPlayerCharacter::CharacterSpeedScalable()
{
	if (!m_bFlying)
		return BaseClass::CharacterSpeedScalable();

	float flDebugBonus = 1;
#ifdef _DEBUG
	if (m_bHyperdrive)
		flDebugBonus = 10;
#endif

	CPlanet* pPlanet = GetNearestPlanet(FINDPLANET_ANY);

	CScalableFloat flMaxSpeed = CScalableFloat(10.0f, SCALE_MEGAMETER);

	CScalableFloat flDistance = (pPlanet->GetGlobalScalableOrigin() - GetGlobalScalableOrigin()).Length();
	CScalableFloat flCloseOrbit = pPlanet->GetRadius()+pPlanet->GetCloseOrbit();

	if (flDistance < flCloseOrbit)
	{
		CScalableFloat flAtmosphereSpeed = CScalableFloat(500.0f, SCALE_KILOMETER);

		CScalableFloat flAtmosphere = pPlanet->GetRadius()+pPlanet->GetAtmosphereThickness();

		if (flDistance < flAtmosphere)
		{
			CScalableFloat flGroundSpeed = CScalableFloat(1.0f, SCALE_KILOMETER);
			return RemapVal(flDistance, pPlanet->GetRadius(), flAtmosphere, flGroundSpeed, flAtmosphereSpeed) * flDebugBonus;
		}

		if (flDistance > flCloseOrbit)
			return flMaxSpeed * flDebugBonus;

		return RemapVal(flDistance, flAtmosphere, flCloseOrbit, flAtmosphereSpeed, flMaxSpeed) * flDebugBonus;
	}

	if (m_bHyperdrive)
	{
		CScalableFloat flMinSpeed = flMaxSpeed;
		flMaxSpeed = CScalableFloat(50.0f, SCALE_GIGAMETER);
		return RemapVal(flDistance, flCloseOrbit, CScalableFloat(1.0f, SCALE_GIGAMETER), flMinSpeed, flMaxSpeed);
	}
	else
		return flMaxSpeed;
}

bool CPlayerCharacter::ShouldTouch(ISPEntity* pOther)
{
	if (GetGroundEntity() == static_cast<CSPEntity*>(pOther))
		return false;

	return BaseClass::ShouldTouch(pOther);
}

CScalableVector CPlayerCharacter::GetGlobalScalableGravity() const
{
	if (m_bFlying)
		return CScalableVector();

	return ISPEntity::GetGlobalScalableGravity();
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
