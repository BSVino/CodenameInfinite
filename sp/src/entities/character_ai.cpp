#include "sp_character.h"

#include "entities/structures/pallet.h"
#include "entities/structures/mine.h"
#include "entities/items/pickup.h"
#include "entities/enemies/eater.h"
#include "entities/sp_playercharacter.h"
#include "ui/command_menu.h"
#include "entities/structures/spire.h"
#include "planet/terrain_lumps.h"
#include "voxel/voxel_tree.h"

void CSPCharacter::TaskThink()
{
	if (GetControllingPlayer())
		return;

	if (GameData().GetCommandMenu())
	{
		// Face the player and await instructions.

		Vector vecDirection = (GameData().GetCommandMenu()->GetPlayerCharacter()->GetGlobalOrigin() - GetGlobalOrigin()).GetMeters().Normalized();

		if (HasMoveParent())
			SetViewAngles(VectorAngles(GetMoveParent()->GetGlobalTransform().InvertedRT().GetMeters().TransformVector(vecDirection)));
		else
			SetViewAngles(VectorAngles(vecDirection));

		m_vecGoalVelocity = Vector(0, 0, 0);
		return;
	}

	if (m_eTask)
		m_vecGoalVelocity = Vector(0, 0, 0);

	if (m_eTask == TASK_BUILD)
	{
		CStructure* pBuild = m_hBuild;

		if (!pBuild || !pBuild->IsUnderConstruction() || pBuild->IsOccupied())
			m_hBuild = pBuild = FindNearestBuildStructure();

		CVoxelTree* pVoxelTree = nullptr;
		if (!GameData().GetLump())
			pVoxelTree = GameData().GetLump()->GetVoxelTree();

		if (m_hWaitingFor)
		{
			if (!m_hWaitingFor->IsOccupied())
				m_hWaitingFor = nullptr;
		}

		if (pBuild && !m_hWaitingFor)
		{
			if (!MoveTo(pBuild))
				return;

			pBuild->PerformStructureTask(this);
			m_hWaitingFor = pBuild;
		}
		else if (pVoxelTree && !m_hWaitingFor)
		{
			if (m_vecBuildDesignation == IVector(0, 0, 0) || !pVoxelTree->GetDesignation(m_vecBuildDesignation))
				m_vecBuildDesignation = FindNearbyDesignation(pVoxelTree);

			if (m_vecBuildDesignation == IVector(0, 0, 0))
				return;

			item_t eDesignation = pVoxelTree->GetDesignation(m_vecBuildDesignation);
			if (IsHoldingABlock())
			{
				TVector vecBlockLocal = pVoxelTree->ToLocalCoordinates(m_vecBuildDesignation) + pVoxelTree->GetTreeToPlanet().TransformVector(Vector(0.5f, 0.5f, 0.5f));
				TVector vecBlockGlobal = GameData().GetPlanet()->GetGlobalTransform() * vecBlockLocal;

				if (!MoveTo(vecBlockGlobal))
					return;

				PlaceBlock((item_t)m_aiInventoryTypes[0], vecBlockLocal);
			}
			else
			{
				CPallet* pPallet = FindNearestPallet(eDesignation, false);
				if (!pPallet)
					return;

				if (!MoveTo(pPallet))
					return;

				pPallet->GiveBlocks(1, this);
			}
		}
	}
	else if (m_eTask == TASK_MINE)
	{
		if (m_hWaitingFor)
		{
			if (!m_hWaitingFor->IsOccupied())
				m_hWaitingFor = nullptr;
		}

		if (IsHoldingABlock())
		{
			// Let's head to the pallete
			CStructure* pPallet = FindNearestPallet(m_aiInventoryTypes[0], true);
			if (!pPallet)
				return;

			if (!MoveTo(pPallet))
				return;

			GiveBlocks(m_aiInventoryTypes[0], 1, pPallet);
		}
		else if (!m_hWaitingFor)
		{
			// Let's head to the mine
			CStructure* pMine = m_hMine;

			if (!pMine || pMine->IsOccupied())
				m_hMine = pMine = FindNearestMine();

			if (!pMine)
				return;

			if (!MoveTo(pMine))
				return;

			CPickup* pPickup = FindNearbyPickup();

			if (pPickup)
			{
				MoveTo(pPickup, 1);
				return;
			}

			pMine->PerformStructureTask(this);
			m_hWaitingFor = pMine;
		}
	}
	else if (m_eTask == TASK_FOLLOWME)
	{
		CPlayerCharacter* pOwner = GameData().GetPlayerOwner()->GetPlayerCharacter();
		if (pOwner)
			MoveTo(pOwner);
	}
	else if (m_eTask == TASK_ATTACK)
	{
		CBaseEntity* pEnemy = m_hEnemy;

		if (!pEnemy)
			m_hEnemy = pEnemy = FindBestEnemy();

		if (!pEnemy)
			return;

		MoveTo(pEnemy, 0);

		if ((pEnemy->GetGlobalOrigin() - GetGlobalOrigin()).Length() < MeleeAttackSphereRadius()*2)
			MeleeAttack();
	}
}

bool CSPCharacter::MoveTo(CBaseEntity* pTarget, float flMoveDistance)
{
	if (!pTarget)
		return false;

	return MoveTo(pTarget->GetGlobalOrigin(), flMoveDistance);
}

bool CSPCharacter::MoveTo(const TVector& vecTarget, float flMoveDistance)
{
	Vector vecToTarget = (vecTarget - GetGlobalOrigin()).GetMeters();
	float flDistance = vecToTarget.Length();
	if (flDistance < flMoveDistance)
		return true;

	Vector vecDirection = vecToTarget/flDistance;

	if (HasMoveParent())
		SetViewAngles(VectorAngles(GetMoveParent()->GetGlobalTransform().InvertedRT().GetMeters().TransformVector(vecDirection)));
	else
		SetViewAngles(VectorAngles(vecDirection));

	// Test to see if the way is blocked.
	Vector vecVelocity = (GameData().TransformVectorLocalToPhysics(AngleVector(GetViewAngles()))).Normalized();
	Vector vecOrigin = GetPhysicsTransform().GetTranslation();

	CTraceResult tr;
	GamePhysics()->TraceEntity(tr, this, vecOrigin, vecOrigin + vecVelocity * 0.4f);

	// If the way is blocked, try to move up and over the object.
	if (tr.m_flFraction < 1)
	{
		if (tr.m_vecNormal.y < 0.01)
			vecDirection = GetUpVector();
		else if (tr.m_vecNormal.Dot(vecVelocity) < 0.7f)
			vecDirection = GetUpVector();
	}

	if (HasMoveParent())
		SetViewAngles(VectorAngles(GetMoveParent()->GetGlobalTransform().InvertedRT().GetMeters().TransformVector(vecDirection.Normalized())));
	else
		SetViewAngles(VectorAngles(vecDirection));

	m_vecGoalVelocity = Vector(1, 0, 0);

	return false;
}

CStructure* CSPCharacter::FindNearestBuildStructure() const
{
	CSpire* pSpire = GetNearestSpire();
	if (!pSpire)
		return nullptr;

	CStructure* pNearestStructure = nullptr;
	for (size_t i = 0; i < pSpire->GetStructures().size(); i++)
	{
		CStructure* pStructure = pSpire->GetStructures()[i];
		if (!pStructure)
			continue;

		if (!pStructure->IsUnderConstruction())
			continue;

		if (pStructure->IsOccupied())
			continue;

		if (!pNearestStructure)
		{
			pNearestStructure = pStructure;
			continue;
		}

		if ((pStructure->GetGlobalOrigin() - GetGlobalOrigin()).LengthSqr() < (pNearestStructure->GetGlobalOrigin() - GetGlobalOrigin()).LengthSqr())
			pNearestStructure = pStructure;
	}

	return pNearestStructure;
}

const IVector CSPCharacter::FindNearbyDesignation(CVoxelTree* pTree) const
{
	if (!pTree)
		return IVector(0, 0, 0);

	return pTree->FindNearbyDesignation(GetLocalOrigin());
}

CPallet* CSPCharacter::FindNearestPallet(item_t eBlock, bool bEmptyOK) const
{
	CSpire* pSpire = GetNearestSpire();
	if (!pSpire)
		return nullptr;

	CPallet* pStructure = nullptr;
	for (size_t i = 0; i < pSpire->GetStructures().size(); i++)
	{
		CPallet* pPallet = dynamic_cast<CPallet*>(pSpire->GetStructures()[i].GetPointer());
		if (!pPallet)
			continue;

		if (pPallet->GetBlockQuantity() && pPallet->GetBlockType() != eBlock)
			continue;

		if (!pPallet->GetBlockQuantity())
			continue;

		if (pPallet->IsUnderConstruction())
			continue;

		if (pPallet->GetBlockQuantity() == pPallet->GetBlockCapacity())
			continue;

		if (!pStructure)
		{
			pStructure = pPallet;
			continue;
		}

		if ((pPallet->GetGlobalOrigin() - GetGlobalOrigin()).LengthSqr() < (pStructure->GetGlobalOrigin() - GetGlobalOrigin()).LengthSqr())
			pStructure = pPallet;
	}

	if (pStructure)
		return pStructure;

	if (!bEmptyOK)
		return nullptr;

	pStructure = nullptr;
	for (size_t i = 0; i < pSpire->GetStructures().size(); i++)
	{
		CPallet* pPallet = dynamic_cast<CPallet*>(pSpire->GetStructures()[i].GetPointer());
		if (!pPallet)
			continue;

		if (pPallet->GetBlockQuantity() && pPallet->GetBlockType() != eBlock)
			continue;

		if (pPallet->IsUnderConstruction())
			continue;

		if (pPallet->GetBlockQuantity() == pPallet->GetBlockCapacity())
			continue;

		if (!pStructure)
		{
			pStructure = pPallet;
			continue;
		}

		if ((pPallet->GetGlobalOrigin() - GetGlobalOrigin()).LengthSqr() < (pStructure->GetGlobalOrigin() - GetGlobalOrigin()).LengthSqr())
			pStructure = pPallet;
	}

	return pStructure;
}

CMine* CSPCharacter::FindNearestMine() const
{
	CSpire* pSpire = GetNearestSpire();
	if (!pSpire)
		return nullptr;

	CMine* pStructure = nullptr;
	for (size_t i = 0; i < pSpire->GetMines().size(); i++)
	{
		CMine* pMine = pSpire->GetMines()[i];
		if (!pMine)
			continue;

		if (pMine->IsOccupied())
			continue;

		if (pMine->IsUnderConstruction())
			continue;

		if (!pStructure)
		{
			pStructure = pMine;
			continue;
		}

		if ((pMine->GetGlobalOrigin() - GetGlobalOrigin()).LengthSqr() < (pStructure->GetGlobalOrigin() - GetGlobalOrigin()).LengthSqr())
			pStructure = pMine;
	}

	return pStructure;
}

CPickup* CSPCharacter::FindNearbyPickup() const
{
	for (size_t i = 0; i < GameServer()->GetMaxEntities(); i++)
	{
		CBaseEntity* pEntity = CBaseEntity::GetEntity(i);
		if (!pEntity)
			continue;

		if (pEntity->GameData().GetPlanet() != GameData().GetPlanet())
			continue;

		CPickup* pPickup = dynamic_cast<CPickup*>(pEntity);
		if (!pPickup)
			continue;

		if ((pPickup->GetGlobalOrigin() - GetGlobalOrigin()).LengthSqr() > TFloat(4).Squared())
			continue;

		return pPickup;
	}

	return nullptr;
}

CBaseEntity* CSPCharacter::FindBestEnemy() const
{
	CBaseEntity* pEnemy = nullptr;

	for (size_t i = 0; i < GameServer()->GetMaxEntities(); i++)
	{
		CBaseEntity* pEntity = CBaseEntity::GetEntity(i);
		if (!pEntity)
			continue;

		CStructure* pStructure = dynamic_cast<CStructure*>(pEntity);
		CSPCharacter* pCharacter = dynamic_cast<CSPCharacter*>(pEntity);
		if (!pStructure && !pCharacter)
			continue;

		if (dynamic_cast<CEater*>(pEntity))
			continue;

		if (!pEnemy)
		{
			pEnemy = pEntity;
			continue;
		}

		if ((pEntity->GetGlobalOrigin() - GetGlobalOrigin()).LengthSqr() < (pEnemy->GetGlobalOrigin() - GetGlobalOrigin()).LengthSqr())
			pEnemy = pEntity;
	}

	return pEnemy;
}

void CSPCharacter::SetTask(task_t eTask)
{
	m_eTask = eTask;

	m_hBuild = nullptr;
	m_hWaitingFor = nullptr;
	m_hMine = nullptr;
	m_hEnemy = nullptr;
}
