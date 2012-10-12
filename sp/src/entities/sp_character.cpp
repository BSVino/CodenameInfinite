#include "sp_character.h"

#include <tinker/application.h>

#include "planet/planet.h"
#include "planet/planet_terrain.h"
#include "planet/terrain_chunks.h"
#include "entities/sp_game.h"
#include "sp_renderer.h"
#include "entities/sp_playercharacter.h"
#include "entities/structures/spire.h"
#include "entities/star.h"
#include "entities/items/pickup.h"

REGISTER_ENTITY(CSPCharacter);

NETVAR_TABLE_BEGIN(CSPCharacter);
	NETVAR_DEFINE(CEntityHandle, m_hNearestPlanet);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CSPCharacter);
	SAVEDATA_DEFINE(CSaveData::DATA_NETVAR, CEntityHandle<CPlanet>, m_hNearestPlanet);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, float, m_flNextPlanetCheck);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, float, m_flLastEnteredAtmosphere);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, float, m_flRollFromSpace);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYARRAY, size_t, m_aiInventory);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CSPCharacter);
INPUTS_TABLE_END();

const int CSPCharacter::MAX_INVENTORY = 16;

CSPCharacter::CSPCharacter()
{
	m_flNextPlanetCheck = 0;
	m_flNextSpireCheck = 0;
	m_flLastEnteredAtmosphere = -1000;

	memset(&m_aiInventory[0], 0, sizeof(m_aiInventory));
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
		GamePhysics()->SetEntityUpVector(this, Vector(0, 1, 0));

	if (pPlanet && ApplyGravity())
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
	else
		SetGlobalGravity(Vector(0, 0, 0));
}

bool CSPCharacter::ShouldRender() const
{
	if (SPGame()->GetSPRenderer()->GetRenderingScale() != SCALE_RENDER)
		return false;

	return BaseClass::ShouldRender();
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

bool CSPCharacter::CanPickUp(CPickup* pPickup) const
{
	if (!pPickup)
		return false;

	if (m_aiInventory[pPickup->GetItem()] >= MAX_INVENTORY)
		return false;

	return true;
}

void CSPCharacter::PickUp(CPickup* pPickup)
{
	if (!pPickup)
		return;

	item_t eItem = pPickup->GetItem();

	if (m_aiInventory[eItem] >= MAX_INVENTORY)
		return;

	pPickup->Delete();

	m_aiInventory[eItem]++;
}

const TMatrix CSPCharacter::GetMovementVelocityTransform() const
{
	CPlanet* pPlanet = GetNearestPlanet();

	TMatrix m;
	m.SetAngles(GetViewAngles());
	if (pPlanet && pPlanet->GetChunkManager()->HasGroupCenter())
		m = TMatrix(pPlanet->GetChunkManager()->GetPlanetToGroupCenterTransform()) * m;

	return m;
}

void CSPCharacter::CharacterMovement(class btCollisionWorld* pWorld, float flDelta)
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
		if (ApplyGravity())
			GamePhysics()->SetEntityGravity(this, Vector(0, -10, 0));
		else
			GamePhysics()->SetEntityGravity(this, Vector(0, 0, 0));

		GamePhysics()->SetEntityUpVector(this, Vector(0, 1, 0));
		GamePhysics()->CharacterMovement(this, pWorld, flDelta);

		// Consider ourselves to have simulated up to this point even though Simulate() isn't run.
		m_flMoveSimulationTime = GameServer()->GetGameTime();
	}
	else
		Simulate();
}

const Matrix4x4 CSPCharacter::GetPhysicsTransform() const
{
	CPlanet* pPlanet = GetNearestPlanet();
	if (!pPlanet)
		return GetLocalTransform();

	if (!pPlanet->GetChunkManager()->HasGroupCenter())
		return GetLocalTransform();

	return m_mGroupTransform;
}

void CSPCharacter::SetPhysicsTransform(const Matrix4x4& m)
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

void CSPCharacter::PostSetLocalTransform(const TMatrix& m)
{
	BaseClass::PostSetLocalTransform(m);

	CPlanet* pPlanet = GetNearestPlanet();
	if (!pPlanet)
		return;

	if (!pPlanet->GetChunkManager()->HasGroupCenter())
		return;

	m_mGroupTransform = pPlanet->GetChunkManager()->GetPlanetToGroupCenterTransform() * m.GetUnits(SCALE_METER);
}

const Vector CSPCharacter::TransformPointPhysicsToLocal(const Vector& v)
{
	CPlanet* pPlanet = GetNearestPlanet();
	if (!pPlanet)
	{
		TAssert(pPlanet);
		return v;
	}

	if (!pPlanet->GetChunkManager()->HasGroupCenter())
	{
		TAssert(pPlanet->GetChunkManager()->HasGroupCenter());
		return v;
	}

	return pPlanet->GetChunkManager()->GetGroupCenterToPlanetTransform() * v;
}

const Vector CSPCharacter::TransformVectorLocalToPhysics(const Vector& v)
{
	CPlanet* pPlanet = GetNearestPlanet();
	if (!pPlanet)
	{
		TAssert(pPlanet);
		return v;
	}

	if (!pPlanet->GetChunkManager()->HasGroupCenter())
	{
		TAssert(pPlanet->GetChunkManager()->HasGroupCenter());
		return v;
	}

	return pPlanet->GetChunkManager()->GetPlanetToGroupCenterTransform().TransformVector(v);
}

void CSPCharacter::SetGroundEntity(CBaseEntity* pEntity)
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

void CSPCharacter::SetGroundEntityExtra(size_t iExtra)
{
	SetGroundEntity(GetNearestPlanet());
}

CPlanet* CSPCharacter::GetNearestPlanet(findplanet_t eFindPlanet)
{
	if (GameServer()->GetGameTime() >= m_flNextPlanetCheck)
	{
		CPlanet* pNearestPlanet = FindNearestPlanet();

		if (!pNearestPlanet)
			return nullptr;

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

CSpire* CSPCharacter::GetNearestSpire() const
{
	if (GameServer()->GetGameTime() > m_flNextSpireCheck)
	{
		CSpire* pNearestSpire = FindNearestSpire();

		m_hNearestSpire = pNearestSpire;
		m_flNextSpireCheck = GameServer()->GetGameTime() + 0.3f;
	}

	return m_hNearestSpire;
}

CSpire* CSPCharacter::FindNearestSpire() const
{
	CSpire* pNearestSpire = NULL;
	CScalableFloat flNearestDistance;

	for (size_t i = 0; i < GameServer()->GetMaxEntities(); i++)
	{
		CSpire* pSpire = dynamic_cast<CSpire*>(CBaseEntity::GetEntity(i));
		if (!pSpire)
			continue;

		CScalableFloat flDistance = (pSpire->GetGlobalOrigin() - GetGlobalOrigin()).Length();

		if (pNearestSpire == NULL)
		{
			pNearestSpire = pSpire;
			flNearestDistance = flDistance;
			continue;
		}

		if (flDistance > flNearestDistance)
			continue;

		pNearestSpire = pSpire;
		flNearestDistance = flDistance;
	}

	return pNearestSpire;
}

void CSPCharacter::ClearNearestSpire()
{
	m_flNextSpireCheck = 0;
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
	CPlanet* pPlanet = FindNearestPlanet();
	if (!pPlanet)
		return;

	CScalableVector vecPlanetGlobal = pPlanet->GetGlobalOrigin();

	CStar* pNearestStar = nullptr;
	for (size_t i = 0; i < GameServer()->GetMaxEntities(); i++)
	{
		CBaseEntity* pEntity = CBaseEntity::GetEntity(i);
		if (!pEntity)
			continue;

		CStar* pStar = dynamic_cast<CStar*>(pEntity);
		if (!pStar)
			continue;

		if (pNearestStar == NULL)
		{
			pNearestStar = pStar;
			continue;
		}

		if ((pStar->GetGlobalOrigin()-vecPlanetGlobal).LengthSqr() < (pNearestStar->GetGlobalOrigin()-vecPlanetGlobal).LengthSqr())
			pNearestStar = pStar;
	}

	TAssert(pNearestStar);
	if (!pNearestStar)
		return;

	SetMoveParent(nullptr);

	Vector vecToStar = (pNearestStar->GetGlobalOrigin() - pPlanet->GetGlobalOrigin()).Normalized();

	// Assumes planet spin and that this star system's up vector is 0,1,0
	Vector vecToDawn = -vecToStar.Cross(Vector(0, 1, 0));

	CScalableVector vecGlobalDawn = pPlanet->GetGlobalOrigin() + CScalableVector(vecToDawn) * pPlanet->GetRadius();
	DoubleVector vecPlayerLocal = (pPlanet->GetGlobalToLocalTransform() * vecGlobalDawn).GetUnits(pPlanet->GetScale());

	SetMoveParent(pPlanet);

	size_t iTerrain;
	DoubleVector2D vecApprox2DCoord;
	pPlanet->GetApprox2DPosition(vecPlayerLocal, iTerrain, vecApprox2DCoord);

	DoubleVector vecNewLocalMeters = pPlanet->GetTerrain(iTerrain)->CoordToWorld(vecApprox2DCoord) + pPlanet->GetTerrain(iTerrain)->GenerateOffset(vecApprox2DCoord);

	CScalableVector vecNewLocal(vecNewLocalMeters, pPlanet->GetScale());

	SetLocalOrigin(vecNewLocal);

	pPlanet->RenderUpdate();
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
