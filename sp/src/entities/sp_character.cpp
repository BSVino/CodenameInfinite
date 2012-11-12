#include "sp_character.h"

#include <tinker/application.h>
#include <tinker/renderer/renderingcontext.h>

#include "planet/planet.h"
#include "planet/planet_terrain.h"
#include "planet/terrain_chunks.h"
#include "entities/sp_game.h"
#include "sp_renderer.h"
#include "entities/sp_playercharacter.h"
#include "entities/structures/spire.h"
#include "entities/star.h"
#include "entities/items/pickup.h"
#include "sp_window.h"
#include "ui/hud.h"

REGISTER_ENTITY(CSPCharacter);

NETVAR_TABLE_BEGIN(CSPCharacter);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CSPCharacter);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, float, m_flNextPlanetCheck);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, float, m_flLastEnteredAtmosphere);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, float, m_flRollFromSpace);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYARRAY, size_t, m_aiInventorySlots);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYARRAY, item_t, m_aiInventoryTypes);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, task_t, m_eTask);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CEntityHandle<CStructure>, m_hBuild);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, IVector, m_vecBuildDesignation);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CEntityHandle<CStructure>, m_hMine);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CEntityHandle<CStructure>, m_hWaitingForMine);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CEntityHandle<CBaseEntity>, m_hEnemy);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CSPCharacter);
INPUTS_TABLE_END();

CSPCharacter::CSPCharacter()
{
	m_flNextPlanetCheck = 0;
	m_flNextSpireCheck = 0;
	m_flLastEnteredAtmosphere = -1000;

	memset(&m_aiInventorySlots[0], 0, sizeof(m_aiInventorySlots));
	memset(&m_aiInventoryTypes[0], 0, sizeof(m_aiInventoryTypes));

	m_eTask = TASK_NONE;
}

void CSPCharacter::Spawn()
{
	BaseClass::Spawn();

	SetTotalHealth(10);

	m_flMaxStepSize = TFloat(0.1f, SCALE_METER);
}

void CSPCharacter::Think()
{
	TaskThink();

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

	if (!HasMoveParent() || !pPlanet)
		SetMoveParent(pPlanet);

	if (pPlanet && !bHadParent)
		EnteredAtmosphere();

	if (pPlanet && IsInPhysics() && GamePhysics()->IsEntityAdded(this))
		GamePhysics()->SetEntityUpVector(this, Vector(0, 1, 0));

	if (pPlanet && !IsFlying())
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

void CSPCharacter::ModifyContext(CRenderingContext* pContext) const
{
	BaseClass::ModifyContext(pContext);

	if (GameServer()->GetGameTime() < m_flLastTakeDamage + 0.2)
	{
		if (!pContext->m_pShader)
			pContext->UseProgram("model");

		pContext->SetUniform("vecColor", Vector4D(1, 0, 0, 1));
	}
}

bool CSPCharacter::CanPickUp(CPickup* pPickup) const
{
	if (!pPickup)
		return false;

	for (size_t i = 0; i < MaxSlots(); i++)
	{
		if (m_aiInventorySlots[i] == 0)
			return true;

		if (m_aiInventoryTypes[i] == pPickup->GetItem() && m_aiInventorySlots[i] < MaxInventory())
			return true;
	}

	return false;
}

void CSPCharacter::PickUp(CPickup* pPickup)
{
	if (!pPickup)
		return;

	item_t eItem = pPickup->GetItem();

	if (!CanPickUp(pPickup))
		return;

	pPickup->Delete();

	TakeBlocks(pPickup->GetItem(), 1);

	SPWindow()->GetHUD()->BuildMenus();
}

bool CSPCharacter::HasBlocks() const
{
	for (size_t i = 0; i < MaxSlots(); i++)
	{
		if (m_aiInventorySlots[i] && m_aiInventoryTypes[i] <= ITEM_BLOCKS_TOTAL)
			return true;
	}

	return false;
}

size_t CSPCharacter::GetInventory(item_t eItem) const
{
	if (eItem < ITEM_NONE)
		return 0;

	if (eItem >= ITEM_TOTAL)
		return 0;

	size_t iTotal = 0;

	for (size_t i = 0; i < MaxSlots(); i++)
	{
		if (m_aiInventoryTypes[i] == eItem)
			iTotal += m_aiInventorySlots[i];
	}

	return iTotal;
}

bool CSPCharacter::PlaceBlock(item_t eItem, const CScalableVector& vecLocal)
{
	if (!GetInventory(eItem))
		return false;

	if (eItem >= ITEM_BLOCKS_TOTAL)
		return false;

	CSpire* pSpire = GetNearestSpire();
	if (!pSpire)
		return false;

	CVoxelTree* pVoxel = pSpire->GetVoxelTree();

	if (!pVoxel->PlaceBlock(eItem, vecLocal))
		return false;

	RemoveBlocks(eItem, 1);

	return true;
}

bool CSPCharacter::PlaceBlock(item_t eItem, const IVector& vecBlock)
{
	if (!GetInventory(eItem))
		return false;

	if (eItem >= ITEM_BLOCKS_TOTAL)
		return false;

	CSpire* pSpire = GetNearestSpire();
	if (!pSpire)
		return false;

	CVoxelTree* pVoxel = pSpire->GetVoxelTree();

	if (!pVoxel->PlaceBlock(eItem, vecBlock))
		return false;

	RemoveBlocks(eItem, 1);

	return true;
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
		if (IsFlying())
			GamePhysics()->SetEntityGravity(this, Vector(0, 0, 0));
		else
			GamePhysics()->SetEntityGravity(this, Vector(0, -10, 0));

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

	return GameData().GetGroupTransform();
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

	GameData().SetGroupTransform(m);

	TMatrix mLocalTransform(pPlanet->GetChunkManager()->GetGroupCenterToPlanetTransform() * m);

	CBaseEntity* pMoveParent = GetMoveParent();
	TAssert(pMoveParent);
	if (pMoveParent)
	{
		while (pMoveParent != pPlanet)
		{
			mLocalTransform = pMoveParent->GetLocalTransform().InvertedRT() * mLocalTransform;
			pMoveParent = pMoveParent->GetMoveParent();
		}
	}

	SetLocalTransform(mLocalTransform);
}

void CSPCharacter::PostSetLocalTransform(const TMatrix& m)
{
	BaseClass::PostSetLocalTransform(m);

	CPlanet* pPlanet = GetNearestPlanet();
	if (!pPlanet)
		return;

	if (!pPlanet->GetChunkManager()->HasGroupCenter())
		return;

	TMatrix mLocalTransform = m;

	CBaseEntity* pMoveParent = GetMoveParent();
	if (pMoveParent)
	{
		while (pMoveParent != pPlanet)
		{
			mLocalTransform = pMoveParent->GetLocalTransform() * mLocalTransform;
			pMoveParent = pMoveParent->GetMoveParent();
		}
	}
	else
		mLocalTransform = pPlanet->GetGlobalToLocalTransform() * m;

	GameData().SetGroupTransform(pPlanet->GetChunkManager()->GetPlanetToGroupCenterTransform() * mLocalTransform.GetUnits(SCALE_METER));
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
			GameData().SetPlanet(pNearestPlanet);
		else
			GameData().SetPlanet(nullptr);

		m_flNextPlanetCheck = GameServer()->GetGameTime() + 0.3f;
	}

	if (eFindPlanet != FINDPLANET_INATMOSPHERE)
	{
		if (GameData().GetPlanet() != nullptr)
			return GameData().GetPlanet();

		CPlanet* pNearestPlanet = FindNearestPlanet();
		if (eFindPlanet == FINDPLANET_ANY)
			return pNearestPlanet;

		CScalableFloat flDistance = (pNearestPlanet->GetGlobalOrigin() - GetGlobalOrigin()).Length() - pNearestPlanet->GetRadius();
		if (eFindPlanet == FINDPLANET_CLOSEORBIT && flDistance > pNearestPlanet->GetCloseOrbit())
			return NULL;
		else
			return pNearestPlanet;
	}

	return GameData().GetPlanet();
}

CPlanet* CSPCharacter::GetNearestPlanet(findplanet_t eFindPlanet) const
{
	if (eFindPlanet != FINDPLANET_INATMOSPHERE)
	{
		if (GameData().GetPlanet() != NULL)
			return GameData().GetPlanet();

		CPlanet* pNearestPlanet = FindNearestPlanet();
		if (eFindPlanet == FINDPLANET_ANY)
			return pNearestPlanet;

		CScalableFloat flDistance = (pNearestPlanet->GetGlobalOrigin() - GetGlobalOrigin()).Length() - pNearestPlanet->GetRadius();
		if (eFindPlanet == FINDPLANET_CLOSEORBIT && flDistance > pNearestPlanet->GetCloseOrbit())
			return NULL;
		else
			return pNearestPlanet;
	}

	return GameData().GetPlanet();
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
	{
		if (GetMoveParent() == pNearestPlanet)
			return Vector(GetLocalOrigin()).Normalized();
		else
			return Vector(pNearestPlanet->GetGlobalToLocalTransform() * GetGlobalOrigin()).Normalized();
	}

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
	// 160 centimeters
	return CScalableFloat(1.6f, SCALE_METER);
}

CScalableFloat CSPCharacter::JumpStrength()
{
	return CScalableFloat(5.0f, SCALE_METER);
}

CScalableFloat CSPCharacter::CharacterSpeed()
{
	return CScalableFloat(5.0f, SCALE_METER);
}

const TVector CSPCharacter::MeleeAttackSphereCenter() const
{
	Vector vecForward = GameData().TransformVectorLocalToPhysics(AngleVector(GetViewAngles()));
	return GetPhysicsTransform().GetTranslation() + GetPhysicsTransform().GetUpVector()*((float)EyeHeight()*0.7f) + vecForward * MeleeAttackSphereRadius();
}

size_t CSPCharacter::GiveBlocks(item_t eBlock, size_t iNumber, CBaseEntity* pGiveTo)
{
	iNumber = std::min(GetInventory(eBlock), iNumber);

	size_t iTaken = 0;

	CStructure* pStructure = dynamic_cast<CStructure*>(pGiveTo);
	if (pStructure)
	{
		TAssert(pStructure->TakesBlocks());
		if (pStructure->TakesBlocks())
			iTaken = pStructure->TakeBlocks(eBlock, iNumber);
		else
			return 0;
	}
	else
	{
		TAssert(dynamic_cast<CSPCharacter*>(pGiveTo));
		CSPCharacter* pCharacter = static_cast<CSPCharacter*>(pGiveTo);
		TAssert(pCharacter->TakesBlocks());
		if (pCharacter->TakesBlocks())
			iTaken = pCharacter->TakeBlocks(eBlock, iNumber);
		else
			return 0;
	}

	size_t iTotalTaken = RemoveBlocks(eBlock, iTaken);
	TAssert(iTotalTaken == iTaken);

	SPWindow()->GetHUD()->BuildMenus();

	return iTaken;
}

size_t CSPCharacter::TakeBlocks(item_t eBlock, size_t iNumber)
{
	size_t iTaken = 0;

	// First go through looking for slots that have this thing.
	for (size_t i = 0; i < MaxSlots(); i++)
	{
		if (m_aiInventoryTypes[i] == eBlock && m_aiInventorySlots[i])
		{
			size_t iGiveToThisPile = iNumber;

			if (iGiveToThisPile + m_aiInventorySlots[i] > MaxInventory())
				iGiveToThisPile = MaxInventory() - m_aiInventorySlots[i];

			m_aiInventorySlots[i] += iGiveToThisPile;
			iNumber -= iGiveToThisPile;
			iTaken += iGiveToThisPile;

			if (iNumber == 0)
			{
				SPWindow()->GetHUD()->BuildMenus();
				return iTaken;
			}
		}
	}

	// Now just pile in an empty slot.
	for (size_t i = 0; i < MaxSlots(); i++)
	{
		if (!m_aiInventorySlots[i])
		{
			m_aiInventoryTypes[i] = eBlock;

			size_t iGiveToThisPile = iNumber;

			if (iGiveToThisPile + m_aiInventorySlots[i] > MaxInventory())
				iGiveToThisPile = MaxInventory() - m_aiInventorySlots[i];

			m_aiInventorySlots[i] += iGiveToThisPile;
			iNumber -= iGiveToThisPile;
			iTaken += iGiveToThisPile;

			if (iNumber == 0)
			{
				SPWindow()->GetHUD()->BuildMenus();
				return iTaken;
			}
		}
	}

	SPWindow()->GetHUD()->BuildMenus();
	return iTaken;
}

size_t CSPCharacter::RemoveBlocks(item_t eBlock, size_t iNumber)
{
	size_t iRemoved = 0;
	for (size_t i = 0; i < MaxSlots(); i++)
	{
		if (m_aiInventoryTypes[i] == eBlock && m_aiInventorySlots[i])
		{
			size_t iTakeFromThisPile = iNumber;

			if (iTakeFromThisPile > m_aiInventorySlots[i])
				iTakeFromThisPile = m_aiInventorySlots[i];

			m_aiInventorySlots[i] -= iTakeFromThisPile;
			iNumber -= iTakeFromThisPile;
			iRemoved += iTakeFromThisPile;

			if (iNumber == 0)
				return iRemoved;
		}
	}

	return iRemoved;
}

bool CSPCharacter::IsHoldingABlock() const
{
	TAssert(MaxSlots() == 1);
	return !!m_aiInventorySlots[0];
}

item_t CSPCharacter::GetBlock() const
{
	if (IsHoldingABlock())
		return m_aiInventoryTypes[0];

	return ITEM_NONE;
}

bool CSPCharacter::CanBuildStructure(structure_type eStructure) const
{
	for (size_t i = 1; i < ITEM_BLOCKS_TOTAL; i++)
	{
		if (GetInventory((item_t)i) < SPGame()->StructureCost(eStructure, (item_t)i))
			return false;
	}

	return true;
}

static tstring g_asTaskNames[] =
{
	"None",
	"Build",
	"Mine",
	"Attack",
	"Follow",
};

const tstring& TaskToString(task_t eTask)
{
	if (eTask >= TASK_NONE && eTask < TASK_TOTAL)
		return g_asTaskNames[eTask];

	static tstring sInvalid = "Invalid";
	return sInvalid;
}
