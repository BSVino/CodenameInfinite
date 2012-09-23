#include "sp_playercharacter.h"

#include <tinker/application.h>
#include <tinker/cvar.h>

#include "planet.h"
#include "sp_camera.h"
#include "planet_terrain.h"
#include "terrain_chunks.h"

REGISTER_ENTITY(CPlayerCharacter);

NETVAR_TABLE_BEGIN(CPlayerCharacter);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CPlayerCharacter);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, bool, m_bFlying);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, bool, m_bWalkSpeedOverride);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, bool, m_bHyperdrive);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CSPCamera, m_hCamera);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, double, m_flNextApproximateElevation);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, double, m_flApproximateElevation);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CPlayerCharacter);
INPUTS_TABLE_END();

CPlayerCharacter::CPlayerCharacter()
{
	m_bHyperdrive = false;
	m_bWalkSpeedOverride = false;
	m_bFlying = false;
}

void CPlayerCharacter::Spawn()
{
	m_aabbPhysBoundingBox = AABB(Vector(-0.35f, 0, -0.35f), Vector(0.35f, 2, 0.35f));

	BaseClass::Spawn();

	m_hCamera = GameServer()->Create<CSPCamera>("CSPCamera");
	m_hCamera->SetCharacter(this);

	m_flNextApproximateElevation = 0;
}

CVar sv_approximateelevation("sv_approximateelevation", "1");

void CPlayerCharacter::Think()
{
	if (GameServer()->GetGameTime() > m_flNextApproximateElevation)
		ApproximateElevation();

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

	m_vecMoveVelocity.x = Approach((float)m_vecGoalVelocity.x, (float)m_vecMoveVelocity.x, (float)GameServer()->GetFrameTime()*4);
	m_vecMoveVelocity.y = Approach((float)m_vecGoalVelocity.y, (float)m_vecMoveVelocity.y, (float)GameServer()->GetFrameTime()*4);
	m_vecMoveVelocity.z = Approach((float)m_vecGoalVelocity.z, (float)m_vecMoveVelocity.z, (float)GameServer()->GetFrameTime()*4);

#ifndef _DEBUG
	if (m_bHyperdrive)
		m_vecMoveVelocity.y = m_vecMoveVelocity.z = 0;
#endif

	if (m_vecMoveVelocity.LengthSqr() > 0.0f)
	{
		CScalableVector vecVelocity = GetLocalVelocity();

		TMatrix m = GetMovementVelocityTransform();

		CScalableVector vecMove = m_vecMoveVelocity * CharacterSpeed();
		vecVelocity = m.TransformVector(vecMove);

		SetLocalVelocity(vecVelocity);
	}
	else
		SetLocalVelocity(CScalableVector());
}

const TMatrix CPlayerCharacter::GetMovementVelocityTransform() const
{
	CPlanet* pPlanet = GetNearestPlanet();

	TMatrix m;
	m.SetAngles(GetViewAngles());
	if (pPlanet && pPlanet->GetChunkManager()->HasGroupCenter())
		m = TMatrix(pPlanet->GetChunkManager()->GetPlanetToGroupCenterTransform()) * m;

	return m;
}

void CPlayerCharacter::CharacterMovement(class btCollisionWorld* pWorld, float flDelta)
{
	CPlanet* pPlanet = GetNearestPlanet();
	TAssert(pPlanet);
	if (!pPlanet)
	{
		GamePhysics()->CharacterMovement(this, pWorld, flDelta);
		return;
	}

	if (pPlanet->GetChunkManager()->HasGroupCenter())
	{
		if (m_bFlying)
			GamePhysics()->SetEntityGravity(this, Vector(0, 0, 0));
		else
			GamePhysics()->SetEntityGravity(this, Vector(0, -10, 0));

		GamePhysics()->SetEntityUpVector(this, Vector(0, 1, 0));
		GamePhysics()->SetControllerWalkVelocity(this, GetLocalVelocity());
		GamePhysics()->CharacterMovement(this, pWorld, flDelta);

		// Consider ourselves to have simulated up to this point even though Simulate() isn't run.
		m_flMoveSimulationTime = GameServer()->GetGameTime();
	}
	else
		Simulate();
}

const Matrix4x4 CPlayerCharacter::GetPhysicsTransform() const
{
	CPlanet* pPlanet = GetNearestPlanet();
	if (!pPlanet)
		return GetLocalTransform();

	if (!pPlanet->GetChunkManager()->HasGroupCenter())
		return GetLocalTransform();

	return m_mGroupTransform;
}

void CPlayerCharacter::SetPhysicsTransform(const Matrix4x4& m)
{
	CPlanet* pPlanet = GetNearestPlanet();
	if (!pPlanet)
	{
		SetLocalTransform(TMatrix(m));
		return;
	}

	if (!pPlanet->GetChunkManager()->HasGroupCenter())
	{
		SetLocalTransform(TMatrix(m));
		return;
	}

	m_mGroupTransform = m;

	SetLocalTransform(TMatrix(pPlanet->GetChunkManager()->GetGroupCenterToPlanetTransform() * m));
}

void CPlayerCharacter::SetGroundEntity(CBaseEntity* pEntity)
{
	// Overridden so that we don't mess with the move parent.
	if ((CBaseEntity*)m_hGround == pEntity)
		return;

	m_hGround = pEntity;

	if (pEntity)
	{
		if (GetNearestPlanet())
			TAssert(pEntity == GetNearestPlanet() || pEntity->GetMoveParent() == GetNearestPlanet() || (pEntity->GetMoveParent() && pEntity->GetMoveParent()->GetMoveParent() == GetNearestPlanet()));

		SetMoveParent(pEntity);
	}
	else
		SetMoveParent(GetNearestPlanet());
}

void CPlayerCharacter::SetGroundEntityExtra(size_t iExtra)
{
	SetGroundEntity(GetNearestPlanet());
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
}

void CPlayerCharacter::StopFlying()
{
	m_bFlying = false;
}

void CPlayerCharacter::EnteredAtmosphere()
{
	BaseClass::EnteredAtmosphere();

	ApproximateElevation();
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
			CScalableFloat flGroundSpeed = CScalableFloat(0.1f, SCALE_KILOMETER);
			return RemapValClamped(flDistance, CScalableFloat(m_flApproximateElevation, pPlanet->GetScale()), flAtmosphere, flGroundSpeed, flAtmosphereSpeed) * flDebugBonus;
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

void CPlayerCharacter::ApproximateElevation()
{
	CPlanet* pPlanet = GetNearestPlanet();
	if (!pPlanet)
		return;

	m_flNextApproximateElevation = GameServer()->GetGameTime() + sv_approximateelevation.GetFloat();

	for (size_t i = 0; i < 6; i++)
	{
		DoubleVector2D vecLumpMin;
		DoubleVector2D vecLumpMax;

		if (!pPlanet->GetTerrain(i)->FindAreaNearestToPlayer(pPlanet->LumpDepth(), DoubleVector2D(0, 0), DoubleVector2D(1, 1), vecLumpMin, vecLumpMax))
			continue;

		m_flApproximateElevation = (pPlanet->GetTerrain(i)->CoordToWorld((vecLumpMin + vecLumpMax)/2) + pPlanet->GetTerrain(i)->GenerateOffset((vecLumpMin + vecLumpMax)/2)).Length();
		return;
	}
}

const CScalableVector CPlayerCharacter::GetGlobalGravity() const
{
	if (m_bFlying)
		return CScalableVector();

	return BaseClass::GetGlobalGravity();
}
