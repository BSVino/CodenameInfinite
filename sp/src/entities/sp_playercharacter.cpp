#include "sp_playercharacter.h"

#include <tinker/application.h>
#include <tinker/cvar.h>

#include "planet/planet.h"
#include "entities/sp_camera.h"
#include "planet/planet_terrain.h"
#include "planet/terrain_chunks.h"
#include "bots/helper.h"
#include "items/disassembler.h"
#include "sp_player.h"

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
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CEntityHandle<CHelperBot>, m_hHelper);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CEntityHandle<CDisassembler>, m_hDisassembler);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CEntityHandle<CStructure>, m_hDisassemblingStructure);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, IVector, m_vecDisassemblingBlock);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, double, m_flDisassemblingStart);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, double, m_flNextSurfaceCheck);
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
	m_aabbPhysBoundingBox = AABB(Vector(-0.45f, 0, -0.45f), Vector(0.45f, 1.8f, 0.45f));

	BaseClass::Spawn();

	m_hCamera = GameServer()->Create<CSPCamera>("CSPCamera");
	m_hCamera->SetCharacter(this);

	m_flNextApproximateElevation = 0;
	m_flNextSurfaceCheck = 0;
	m_flDisassemblingStart = 0;

	if (m_hHelper)
		m_hHelper->Delete();

//	m_hHelper = GameServer()->Create<CHelperBot>("CHelperBot");
//	m_hHelper->SetPlayerCharacter(this);

	m_hDisassembler = GameServer()->Create<CDisassembler>("CDisassembler");
	Weapon_Add(m_hDisassembler);
}

CVar sv_approximateelevation("sv_approximateelevation", "0.3");
CVar sv_disassemble_structure_time("sv_disassemble_structure_time", "4.0");
CVar sv_disassemble_block_time("sv_disassemble_block_time", "1.0");

void CPlayerCharacter::Think()
{
	BaseClass::Think();

	if (GameServer()->GetGameTime() > m_flNextApproximateElevation)
		ApproximateElevation();

	if (GameServer()->GetGameTime() > m_flNextSurfaceCheck)
	{
		CPlanet* pPlanet = GetNearestPlanet();
		if (pPlanet)
		{
			if (m_flApproximateElevation < -0.0005)
			{
				double flDistanceUp = -m_flApproximateElevation + 0.0001;
				SetLocalOrigin(GetLocalOrigin() + TVector(DoubleVector(GetLocalUpVector()) * flDistanceUp, pPlanet->GetScale()));

				TMsg("Approximate Elevation detected player under terrain. Raised player to ground level.\n");
			}
			else if (pPlanet->GetChunkManager()->HasGroupCenter())
			{
				CTraceResult tr;
				GamePhysics()->TraceLine(tr, GetPhysicsTransform().GetTranslation(), GetPhysicsTransform().GetTranslation() - Vector(0, 1000, 0), this);

				if (tr.m_flFraction == 1)
				{
					// There is no ground below the player. Is there ground above the player?
					GamePhysics()->TraceLine(tr, GetPhysicsTransform().GetTranslation(), GetPhysicsTransform().GetTranslation() + Vector(0, 1000, 0), this);

					if (tr.m_flFraction != 1)
					{
						Matrix4x4 mNewTransform = GetPhysicsTransform();
						mNewTransform.SetTranslation(tr.m_vecHit + Vector(0, 1, 0));
						SetPhysicsTransform(mNewTransform);

						TMsg("Traceline detected player under terrain. Raised player to ground level.\n");
					}
				}
			}
		}

		m_flNextSurfaceCheck = GameServer()->GetGameTime() + 2;
	}

	if (m_flDisassemblingStart)
	{
		float flDisassembleTime = sv_disassemble_block_time.GetFloat();
		if (m_hDisassemblingStructure)
			flDisassembleTime = sv_disassemble_structure_time.GetFloat();

		if (GameServer()->GetGameTime() > m_flDisassemblingStart + flDisassembleTime)
		{
			m_flDisassemblingStart = 0;

			if (m_hDisassemblingStructure)
				m_hDisassemblingStructure->Delete();
			else if (GetNearestSpire())
				GetNearestSpire()->GetVoxelTree()->RemoveBlock(m_vecDisassemblingBlock);

			m_hDisassembler->EndDisassembly();
		}
		else
		{
			Matrix4x4 mTransform = GetPhysicsTransform();

			Vector vecEye = mTransform.GetTranslation() + Vector(0, 1, 0)*EyeHeight();
			Vector vecDirection = GameData().TransformVectorLocalToPhysics(AngleVector(GetViewAngles()));

			CTraceResult tr;
			GamePhysics()->TraceLine(tr, vecEye, vecEye + vecDirection*4, this);

			if (tr.m_pHit != m_hDisassemblingStructure)
			{
				m_flDisassemblingStart = 0;
				m_hDisassemblingStructure = nullptr;
				m_hDisassembler->EndDisassembly();
			}
		}
	}
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

	CScalableVector vecLocalVelocity = GetLocalVelocity();

	if (m_vecMoveVelocity.LengthSqr() > 0.0f)
	{
		TMatrix m = GetMovementVelocityTransform();

		CScalableVector vecMove = m_vecMoveVelocity * CharacterSpeed();
		vecLocalVelocity = m.TransformVector(vecMove);
	}
	else
		vecLocalVelocity = CScalableVector();

	if (IsInPhysics() && GamePhysics()->IsEntityAdded(this))
	{
		if (GetNearestPlanet() && GetNearestPlanet()->GetChunkManager()->HasGroupCenter())
			GamePhysics()->SetControllerMoveVelocity(this, vecLocalVelocity);
		else
			SetLocalVelocity(vecLocalVelocity);
	}
	else
		SetLocalVelocity(vecLocalVelocity);
}

bool CPlayerCharacter::CanAttack() const
{
	CSPPlayer* pPlayer = static_cast<CSPPlayer*>(GetControllingPlayer());
	TAssert(pPlayer);
	if (!pPlayer)
		return BaseClass::CanAttack();

	if (pPlayer->IsInBlockPlaceMode())
		return false;

	if (pPlayer->IsInConstructionMode())
		return false;

	return BaseClass::CanAttack();
}

void CPlayerCharacter::OnWeaponAdded(CBaseWeapon* pWeapon)
{
	if (!GetEquippedWeapon())
		Weapon_Equip(pWeapon);
}

void CPlayerCharacter::OnWeaponRemoved(CBaseWeapon* pWeapon, bool bWasEquipped)
{
	if (bWasEquipped && m_hDisassembler != pWeapon)
		Weapon_Equip(m_hDisassembler);
}

void CPlayerCharacter::BeginDisassembly(CStructure* pStructure)
{
	TAssert(!m_flDisassemblingStart);
	if (m_flDisassemblingStart)
		return;

	m_hDisassembler->BeginDisassembly();
	m_flDisassemblingStart = GameServer()->GetGameTime();
	m_hDisassemblingStructure = pStructure;
}

void CPlayerCharacter::BeginDisassembly(const IVector& vecBlock)
{
	TAssert(!m_flDisassemblingStart);
	if (m_flDisassemblingStart)
		return;

	m_hDisassembler->BeginDisassembly();
	m_flDisassemblingStart = GameServer()->GetGameTime();
	m_vecDisassemblingBlock = vecBlock;
}

void CPlayerCharacter::EndDisassembly()
{
	TAssert(m_flDisassemblingStart);
	if (m_flDisassemblingStart == 0)
		return;

	m_hDisassembler->EndDisassembly();
	m_flDisassemblingStart = 0;
	m_hDisassemblingStructure = nullptr;
	m_vecDisassemblingBlock = IVector();
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

	m_flApproximateElevation = GetNearestPlanet()->GetAtmosphereThickness().GetUnits(GetNearestPlanet()->GetScale());
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
			CScalableFloat flGroundSpeed = CScalableFloat(20.0f, SCALE_METER);
			CScalableFloat flAboveGroundSpeed = CScalableFloat(200.0f, SCALE_METER);
			float flSpeedRise = 0.0005f;
			if (m_flApproximateElevation < flSpeedRise)
				return RemapValClamped((float)m_flApproximateElevation, 0.0f, flSpeedRise, flGroundSpeed, flAboveGroundSpeed) * flDebugBonus;
			else
				return RemapValClamped(CScalableFloat(m_flApproximateElevation, pPlanet->GetScale()), CScalableFloat(flSpeedRise, pPlanet->GetScale()), pPlanet->GetAtmosphereThickness(), flAboveGroundSpeed, flAtmosphereSpeed) * flDebugBonus;
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

	if (pPlanet->GetCharacterLocalOrigin().LengthSqr() == 0)
		return;

	m_flNextApproximateElevation = GameServer()->GetGameTime() + sv_approximateelevation.GetFloat();

	if (pPlanet->GetChunkManager()->HasGroupCenter())
	{
		float flDistance = 1000;
		CTraceResult tr;
		GamePhysics()->TraceLine(tr, GetPhysicsTransform().GetTranslation(), GetPhysicsTransform().GetTranslation() - Vector(0, flDistance, 0), this);

		if (tr.m_flFraction < 1)
		{
			m_flApproximateElevation = CScalableFloat(tr.m_flFraction * flDistance, SCALE_METER).GetUnits(pPlanet->GetScale());
			return;
		}
	}

	DoubleVector vec3DOrigin = pPlanet->GetCharacterLocalOrigin();

	float flElevation;
	if (pPlanet->FindApproximateElevation(vec3DOrigin, flElevation))
	{
		m_flApproximateElevation = flElevation;
		return;
	}

	size_t iTerrain;
	DoubleVector2D vecApprox2DCoord;
	pPlanet->GetApprox2DPosition(vec3DOrigin, iTerrain, vecApprox2DCoord);

	DoubleVector vecApprox3DCoord = pPlanet->GetTerrain(iTerrain)->CoordToWorld(vecApprox2DCoord) + pPlanet->GetTerrain(iTerrain)->GenerateOffset(vecApprox2DCoord);

	DoubleVector2D vecAdjusted2DCoord;
	pPlanet->GetApprox2DPosition(vecApprox3DCoord, iTerrain, vecAdjusted2DCoord);

	DoubleVector2D vecEstimated2DCoord = vecApprox2DCoord - (vecAdjusted2DCoord - vecApprox2DCoord);

	vec3DOrigin = pPlanet->GetTerrain(iTerrain)->CoordToWorld(vecEstimated2DCoord) + pPlanet->GetTerrain(iTerrain)->GenerateOffset(vecEstimated2DCoord);

	DoubleVector vecOriginNormalized = pPlanet->GetCharacterLocalOrigin().Normalized();

	// Remove any lateral movement from the terrain generation offset.
	Vector vecHit;
	bool bHit = RayIntersectsPlane(Ray(Vector(0, 0, 0), vecOriginNormalized), vec3DOrigin, vecOriginNormalized, &vecHit);
	if (bHit)
	{
		m_flApproximateElevation = vecHit.Distance(pPlanet->GetCharacterLocalOrigin());

		// If the character is nearer to the planet's origin than the hit point then we approximated that we're under the ground.
		if (pPlanet->GetCharacterLocalOrigin().LengthSqr() < vecHit.LengthSqr())
			m_flApproximateElevation = -m_flApproximateElevation;
	}
	else
	{
		m_flApproximateElevation = (TFloat(pPlanet->GetCharacterLocalOrigin().Length(), pPlanet->GetScale()) - pPlanet->GetRadius()).GetUnits(pPlanet->GetScale());
	}
}

const CScalableVector CPlayerCharacter::GetGlobalGravity() const
{
	if (m_bFlying)
		return CScalableVector();

	return BaseClass::GetGlobalGravity();
}

CHelperBot* CPlayerCharacter::GetHelperBot()
{
	return m_hHelper;
}
