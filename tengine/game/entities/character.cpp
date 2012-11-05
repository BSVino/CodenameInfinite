#include "character.h"

#include <matrix.h>
#include <tinker/application.h>
#include <tinker/cvar.h>
#include <game/entities/game.h>
#include <renderer/renderer.h>
#include <renderer/renderingcontext.h>
#include <physics/physics.h>
#include <renderer/game_renderer.h>
#include <game/entities/weapon.h>

#include "player.h"

REGISTER_ENTITY(CCharacter);

NETVAR_TABLE_BEGIN(CCharacter);
	NETVAR_DEFINE(CEntityHandle, m_hControllingPlayer);
	NETVAR_DEFINE(CEntityHandle, m_hGround);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CCharacter);
	SAVEDATA_DEFINE(CSaveData::DATA_NETVAR, int, m_hControllingPlayer);
	SAVEDATA_DEFINE(CSaveData::DATA_NETVAR, CEntityHandle<CBaseEntity>, m_hGround);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, bool, m_bNoClip);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, bool, m_bTransformMoveByView);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, Vector, m_vecGoalVelocity);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, Vector, m_vecMoveVelocity);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, double, m_flMoveSimulationTime);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, double, m_flLastAttack);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, TFloat, m_flMaxStepSize);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, size_t, m_iEquippedWeapon);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYVECTOR, CBaseWeapon, m_ahWeapons);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CCharacter);
INPUTS_TABLE_END();

CCharacter::CCharacter()
{
	m_bTransformMoveByView = true;
	m_bNoClip = false;

	m_flLastSpawn = -1;

	m_iEquippedWeapon = ~0;

	SetMass(60);
}

void CCharacter::Spawn()
{
	BaseClass::Spawn();

	SetTotalHealth(100);

	m_vecMoveVelocity = Vector(0,0,0);
	m_vecGoalVelocity = Vector(0,0,0);

	m_flMoveSimulationTime = 0;

	m_flLastAttack = -1;
	m_flLastSpawn = GameServer()->GetGameTime();

	m_bTakeDamage = true;

	m_flMaxStepSize = 0.2f;

	AddToPhysics(CT_CHARACTER);

	if (GetControllingPlayer())
		GetControllingPlayer()->Instructor_Respawn();
}

void CCharacter::Think()
{
	BaseClass::Think();

	if (m_bNoClip)
		MoveThink_NoClip();
	else
		MoveThink();

	if (IsInPhysics() && GamePhysics()->IsEntityAdded(this))
		return;

	Simulate();
}

void CCharacter::Simulate()
{
	float flSimulationFrameTime = 0.01f;

	// Break simulations up into consistent small steps to preserve accuracy.
	for (; m_flMoveSimulationTime < GameServer()->GetGameTime(); m_flMoveSimulationTime += flSimulationFrameTime)
	{
		TVector vecVelocity = GetLocalVelocity();

		TVector vecLocalOrigin = GetLocalOrigin();
		TVector vecGlobalOrigin = GetGlobalOrigin();

		vecVelocity = vecVelocity * flSimulationFrameTime;

		TVector vecLocalDestination = vecLocalOrigin + vecVelocity;
		TVector vecGlobalDestination = vecLocalDestination;
		if (GetMoveParent())
			vecGlobalDestination = GetMoveParent()->GetGlobalTransform() * vecLocalDestination;

		TVector vecNewLocalOrigin = vecLocalDestination;

		SetLocalOrigin(vecNewLocalOrigin);

		m_flMoveSimulationTime += flSimulationFrameTime;
	}
}

void CCharacter::Move(movetype_t eMoveType)
{
	if (eMoveType == MOVE_FORWARD)
		m_vecGoalVelocity.x = 1;
	else if (eMoveType == MOVE_BACKWARD)
		m_vecGoalVelocity.x = -1;
	else if (eMoveType == MOVE_RIGHT)
		m_vecGoalVelocity.z = 1;
	else if (eMoveType == MOVE_LEFT)
		m_vecGoalVelocity.z = -1;
}

void CCharacter::StopMove(movetype_t eMoveType)
{
	if (eMoveType == MOVE_FORWARD && m_vecGoalVelocity.x > 0.0f)
		m_vecGoalVelocity.x = 0;
	else if (eMoveType == MOVE_BACKWARD && m_vecGoalVelocity.x < 0.0f)
		m_vecGoalVelocity.x = 0;
	else if (eMoveType == MOVE_LEFT && m_vecGoalVelocity.z < 0.0f)
		m_vecGoalVelocity.z = 0;
	else if (eMoveType == MOVE_RIGHT && m_vecGoalVelocity.z > 0.0f)
		m_vecGoalVelocity.z = 0;
}

const TVector CCharacter::GetGoalVelocity()
{
	if (m_vecGoalVelocity.LengthSqr())
		m_vecGoalVelocity.Normalize();

	return m_vecGoalVelocity;
}

void CCharacter::MoveThink()
{
	float flSimulationFrameTime = 0.01f;

	TVector vecGoalVelocity = GetGoalVelocity();

	m_vecMoveVelocity.x = Approach((float)vecGoalVelocity.x, (float)m_vecMoveVelocity.x, (float)GameServer()->GetFrameTime()*(float)CharacterAcceleration());
	m_vecMoveVelocity.y = 0;
	m_vecMoveVelocity.z = Approach((float)vecGoalVelocity.z, (float)m_vecMoveVelocity.z, (float)GameServer()->GetFrameTime()*(float)CharacterAcceleration());

	TVector vecLocalVelocity;

	if (m_vecMoveVelocity.LengthSqr() > 0.0f)
	{
		TVector vecMove = m_vecMoveVelocity * CharacterSpeed();

		if (m_bTransformMoveByView)
		{
			Vector vecUp = GetUpVector();

			if (HasMoveParent() && GetMoveParent()->TransformsChildUp())
			{
				TMatrix mGlobalToLocal = GetMoveParent()->GetGlobalToLocalTransform();
				vecUp = mGlobalToLocal.TransformVector(vecUp);
			}

			TMatrix m = GetMovementVelocityTransform();

			vecLocalVelocity = m.TransformVector(vecMove);
		}
		else
			vecLocalVelocity = vecMove;
	}
	else
		vecLocalVelocity = TVector();

	if (IsInPhysics() && GamePhysics()->IsEntityAdded(this))
		GamePhysics()->SetControllerMoveVelocity(this, vecLocalVelocity);
	else
		SetLocalVelocity(vecLocalVelocity);
}

void CCharacter::MoveThink_NoClip()
{
	float flSimulationFrameTime = 0.01f;

	TVector vecGoalVelocity = GetGoalVelocity();

	m_vecMoveVelocity.x = Approach((float)vecGoalVelocity.x, (float)m_vecMoveVelocity.x, (float)GameServer()->GetFrameTime()*(float)CharacterAcceleration());
	m_vecMoveVelocity.y = 0;
	m_vecMoveVelocity.z = Approach((float)vecGoalVelocity.z, (float)m_vecMoveVelocity.z, (float)GameServer()->GetFrameTime()*(float)CharacterAcceleration());

	if (m_vecMoveVelocity.LengthSqr() > 0.0f)
	{
		TVector vecMove = m_vecMoveVelocity * CharacterSpeed();
		TVector vecLocalVelocity;

		if (m_bTransformMoveByView)
		{
			Vector vecUp = GetUpVector();

			if (HasMoveParent() && GetMoveParent()->TransformsChildUp())
			{
				TMatrix mGlobalToLocal = GetMoveParent()->GetGlobalToLocalTransform();
				vecUp = mGlobalToLocal.TransformVector(vecUp);
			}

			TMatrix m = GetMovementVelocityTransform();

			vecLocalVelocity = m.TransformVector(vecMove);
		}
		else
			vecLocalVelocity = vecMove;

		SetGlobalOrigin(GetGlobalOrigin() + (float)GameServer()->GetFrameTime()*vecLocalVelocity);
	}
}

const TMatrix CCharacter::GetMovementVelocityTransform() const
{
	return TMatrix(GetViewAngles());
}

void CCharacter::CharacterMovement(class btCollisionWorld* pWorld, float flDelta)
{
	GamePhysics()->CharacterMovement(this, pWorld, flDelta);
}

void CCharacter::Jump()
{
	GamePhysics()->CharacterJump(this);
}

void CCharacter::SetNoClip(bool bOn)
{
	m_bNoClip = bOn;

	GamePhysics()->SetControllerMoveVelocity(this, Vector(0, 0, 0));
	GamePhysics()->SetControllerColliding(this, !m_bNoClip);
}

bool CCharacter::CanAttack() const
{
	float flMeleeAttackTime = GetEquippedWeapon()?GetEquippedWeapon()->MeleeAttackTime():MeleeAttackTime();

	if (m_flLastAttack >= 0 && GameServer()->GetGameTime() - m_flLastAttack < flMeleeAttackTime)
		return false;

	return true;
}

void CCharacter::MeleeAttack()
{
	if (!CanAttack())
		return;

	m_flLastAttack = GameServer()->GetGameTime();
	m_vecMoveVelocity = TVector();

	CBaseWeapon* pWeapon = GetEquippedWeapon();

	float flAttackSphereRadius = pWeapon?pWeapon->MeleeAttackSphereRadius():MeleeAttackSphereRadius();
	float flAttackDamage = pWeapon?pWeapon->MeleeAttackDamage():MeleeAttackDamage();
	Vector vecDamageSphereCenter = pWeapon?pWeapon->MeleeAttackSphereCenter():MeleeAttackSphereCenter();

	if (pWeapon)
		pWeapon->MeleeAttack();

	CTraceResult tr;
	GamePhysics()->CheckSphere(tr, flAttackSphereRadius, vecDamageSphereCenter, this);

	for (size_t j = 0; j < tr.m_aHits.size(); j++)
	{
		CBaseEntity* pEntity = tr.m_aHits[j].m_pHit;

		if (!pEntity)
			continue;

		if (pEntity->IsDeleted())
			continue;

		if (!pEntity->TakesDamage())
			continue;

		if (pEntity == this)
			continue;

		if (!Game()->TakesDamageFrom(pEntity, this))
			continue;

		//TMsg(tstring(GetClassName()) + " hit " + tstring(pEntity->GetClassName()) + "\n");
		pEntity->TakeDamage(this, this, DAMAGE_GENERIC, flAttackDamage);
	}
}

bool CCharacter::IsAttacking() const
{
	if (m_flLastAttack < 0)
		return false;

	float flMeleeAttackTime = GetEquippedWeapon()?GetEquippedWeapon()->MeleeAttackTime():MeleeAttackTime();

	return (GameServer()->GetGameTime() - m_flLastAttack < flMeleeAttackTime);
}

void CCharacter::MoveToPlayerStart()
{
	CBaseEntity* pPlayerStart = CBaseEntity::GetEntityByName("*PlayerStart");

	SetMoveParent(NULL);

	if (!pPlayerStart)
	{
		SetGlobalOrigin(Vector(0, 0, 0));
		SetGlobalAngles(EAngle(0, 0, 0));
		return;
	}

	SetGlobalOrigin(pPlayerStart->GetGlobalOrigin());
	m_angView = pPlayerStart->GetGlobalAngles();
}

CVar debug_showplayervectors("debug_showplayervectors", "off");

void CCharacter::PostRender() const
{
	if (!GameServer()->GetRenderer()->IsRenderingTransparent() && debug_showplayervectors.GetBool())
		ShowPlayerVectors();
}

void CCharacter::ShowPlayerVectors() const
{
	TMatrix m = GetGlobalTransform();
	m.SetAngles(m_angView);

	Vector vecUp = GetUpVector();
	Vector vecRight = m.GetForwardVector().Cross(vecUp).Normalized();
	Vector vecForward = vecUp.Cross(vecRight).Normalized();
	m.SetForwardVector(vecForward);
	m.SetUpVector(vecUp);
	m.SetRightVector(vecRight);

	CCharacter* pLocalCharacter = Game()->GetLocalPlayer()->GetCharacter();

	TVector vecEyeHeight = GetUpVector() * EyeHeight();

	CRenderingContext c(GameServer()->GetRenderer(), true);

	c.UseProgram("model");
	c.Translate((GetGlobalOrigin()));
	c.SetColor(Color(255, 255, 255));
	c.BeginRenderDebugLines();
	c.Vertex(Vector(0,0,0));
	c.Vertex((float)EyeHeight() * vecUp);
	c.EndRender();

	if (!GetGlobalVelocity().IsZero())
	{
		c.BeginRenderDebugLines();
		c.Vertex(vecEyeHeight);
		c.Vertex(vecEyeHeight + GetGlobalVelocity());
		c.EndRender();
	}

	c.SetColor(Color(255, 0, 0));
	c.BeginRenderDebugLines();
	c.Vertex(vecEyeHeight);
	c.Vertex(vecEyeHeight + vecForward);
	c.EndRender();

	c.SetColor(Color(0, 255, 0));
	c.BeginRenderDebugLines();
	c.Vertex(vecEyeHeight);
	c.Vertex(vecEyeHeight + vecRight);
	c.EndRender();

	c.SetColor(Color(0, 0, 255));
	c.BeginRenderDebugLines();
	c.Vertex(vecEyeHeight);
	c.Vertex(vecEyeHeight + vecUp);
	c.EndRender();
}

void CCharacter::Weapon_Add(CBaseWeapon* pWeapon)
{
	TAssert(pWeapon);
	if (!pWeapon)
		return;

	for (size_t i = 0; i < m_ahWeapons.size(); i++)
	{
		if (m_ahWeapons[i] == pWeapon)
			return;
	}

	TAssert(!pWeapon->GetOwner());

	m_ahWeapons.push_back(pWeapon);

	pWeapon->SetOwner(this);

	OnWeaponAdded(pWeapon);
}

void CCharacter::Weapon_Remove(CBaseWeapon* pWeapon)
{
	TAssert(pWeapon);
	if (!pWeapon)
		return;

	size_t iIndex = ~0;
	for (size_t i = 0; i < m_ahWeapons.size(); i++)
	{
		if (m_ahWeapons[i] == pWeapon)
		{
			iIndex = i;
			break;
		}
	}

	if (iIndex == ~0)
		return;

	TAssert(pWeapon->GetOwner() == this);

	bool bWasEquipped = (GetEquippedWeaponIndex() == iIndex);
	m_ahWeapons.erase_value(pWeapon);

	pWeapon->SetOwner(nullptr);

	OnWeaponRemoved(pWeapon, bWasEquipped);
}

void CCharacter::Weapon_Equip(CBaseWeapon* pWeapon)
{
	size_t iIndex = ~0;
	for (size_t i = 0; i < m_ahWeapons.size(); i++)
	{
		if (m_ahWeapons[i] == pWeapon)
		{
			iIndex = i;
			break;
		}
	}

	TAssert(iIndex != ~0);
	if (iIndex == ~0)
		return;

	m_iEquippedWeapon = iIndex;
}

CBaseWeapon* CCharacter::GetEquippedWeapon() const
{
	if (m_iEquippedWeapon == ~0)
		return nullptr;

	TAssert(m_iEquippedWeapon < m_ahWeapons.size());
	TAssert(m_ahWeapons[m_iEquippedWeapon]);

	return m_ahWeapons[m_iEquippedWeapon];
}

void CCharacter::OnDeleted(const CBaseEntity* pEntity)
{
	BaseClass::OnDeleted(pEntity);

	const CBaseWeapon* pWeapon = dynamic_cast<const CBaseWeapon*>(pEntity);
	if (pWeapon)
		m_ahWeapons.erase_value(pWeapon);
}

void CCharacter::SetControllingPlayer(CPlayer* pCharacter)
{
	m_hControllingPlayer = pCharacter;
}

CPlayer* CCharacter::GetControllingPlayer() const
{
	return m_hControllingPlayer;
}

CVar sv_noclip_multiplier("sv_noclip_multiplier", "2");

TFloat CCharacter::CharacterSpeed()
{
	if (m_bNoClip)
		return BaseCharacterSpeed() * sv_noclip_multiplier.GetFloat();
	else
		return BaseCharacterSpeed();
}

const TVector CCharacter::MeleeAttackSphereCenter() const
{
	return GetPhysicsTransform().GetTranslation() + GetPhysicsTransform().GetUpVector()*((float)EyeHeight()*0.7f) + AngleVector(GetViewAngles()) * MeleeAttackSphereRadius();
}

void CCharacter::SetGroundEntity(CBaseEntity* pEntity)
{
	if ((CBaseEntity*)m_hGround == pEntity)
		return;

	m_hGround = pEntity;
	SetMoveParent(pEntity);
}

void CCharacter::SetGroundEntityExtra(size_t iExtra)
{
}
