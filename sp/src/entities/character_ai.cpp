#include "sp_character.h"

#include "entities/structures/pallet.h"
#include "entities/structures/mine.h"
#include "entities/items/pickup.h"

void CSPCharacter::TaskThink()
{
	if (m_eTask == TASK_MINE)
	{
		m_vecGoalVelocity = Vector(0, 0, 0);

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
	SetViewAngles(VectorAngles(GetGlobalToLocalTransform().TransformVector(vecDirection)));
	m_vecGoalVelocity = Vector(1, 0, 0);

	return false;
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
