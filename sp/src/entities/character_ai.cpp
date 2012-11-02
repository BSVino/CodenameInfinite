#include "sp_character.h"

#include "entities/structures/pallet.h"
#include "entities/structures/mine.h"
#include "entities/items/pickup.h"
#include "entities/enemies/eater.h"
#include "entities/sp_playercharacter.h"
#include "ui/command_menu.h"

void CSPCharacter::TaskThink()
{
	if (GameData().GetCommandMenu())
	{
		// Face the player and await instructions.

		Vector vecDirection = (GameData().GetCommandMenu()->GetPlayerCharacter()->GetGlobalOrigin() - GetGlobalOrigin()).GetUnits(SCALE_METER).Normalized();

		if (HasMoveParent())
			SetViewAngles(VectorAngles(GetMoveParent()->GetGlobalTransform().InvertedRT().GetUnits(SCALE_METER).TransformVector(vecDirection)));
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

		if (!pBuild || !pBuild->IsUnderConstruction())
			m_hBuild = pBuild = FindNearestBuildStructure();

		if (!pBuild)
			return;

		if (!MoveTo(pBuild))
			return;

		pBuild->PerformStructureTask(this);
	}
	else if (m_eTask == TASK_MINE)
	{
		if (IsHoldingABlock())
		{
			// Let's head to the pallete
			CStructure* pPallet = FindNearestPallet(m_aiInventoryTypes[0]);
			if (!pPallet)
				return;

			if (!MoveTo(pPallet))
				return;

			GiveBlocks(m_aiInventoryTypes[0], 1, pPallet);
		}
		else
		{
			// Let's head to the mine
			CStructure* pMine = m_hMine;

			if (!pMine)
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

	Vector vecToTarget = (pTarget->GetGlobalOrigin() - GetGlobalOrigin()).GetUnits(SCALE_METER);
	float flDistance = vecToTarget.Length();
	if (flDistance < flMoveDistance)
		return true;

	Vector vecDirection = vecToTarget/flDistance;

	if (HasMoveParent())
		SetViewAngles(VectorAngles(GetMoveParent()->GetGlobalTransform().InvertedRT().GetUnits(SCALE_METER).TransformVector(vecDirection)));
	else
		SetViewAngles(VectorAngles(vecDirection));

	m_vecGoalVelocity = Vector(1, 0, 0);

	return false;
}

CStructure* CSPCharacter::FindNearestBuildStructure() const
{
	CStructure* pNearestStructure = nullptr;
	for (size_t i = 0; i < GameServer()->GetMaxEntities(); i++)
	{
		CBaseEntity* pEntity = CBaseEntity::GetEntity(i);
		if (!pEntity)
			continue;

		CStructure* pStructure = dynamic_cast<CStructure*>(pEntity);
		if (!pStructure)
			continue;

		if (!pStructure->IsUnderConstruction())
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

CPallet* CSPCharacter::FindNearestPallet(item_t eBlock) const
{
	CPallet* pStructure = nullptr;
	for (size_t i = 0; i < GameServer()->GetMaxEntities(); i++)
	{
		CBaseEntity* pEntity = CBaseEntity::GetEntity(i);
		if (!pEntity)
			continue;

		CPallet* pPallet = dynamic_cast<CPallet*>(pEntity);
		if (!pPallet)
			continue;

		if (pPallet->GetBlockQuantity() && pPallet->GetBlockType() != eBlock)
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
	CMine* pStructure = nullptr;
	for (size_t i = 0; i < GameServer()->GetMaxEntities(); i++)
	{
		CBaseEntity* pEntity = CBaseEntity::GetEntity(i);
		if (!pEntity)
			continue;

		CMine* pMine = dynamic_cast<CMine*>(pEntity);
		if (!pMine)
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

		CPickup* pPickup = dynamic_cast<CPickup*>(pEntity);
		if (!pPickup)
			continue;

		if ((pPickup->GetGlobalOrigin() - GetGlobalOrigin()).LengthSqr() > TFloat(5).Squared())
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

	m_hMine = nullptr;
	m_hEnemy = nullptr;
}
