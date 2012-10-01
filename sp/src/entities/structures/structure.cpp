#include "structure.h"

#include <game/gameserver.h>

#include "spire.h"
#include "entities/sp_player.h"
#include "entities/sp_playercharacter.h"
#include "mine.h"

REGISTER_ENTITY(CStructure);

NETVAR_TABLE_BEGIN(CStructure);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CStructure);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CSPPlayer, m_hOwner);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CSpire, m_hSpire);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CStructure);
INPUTS_TABLE_END();

void CStructure::SetSpire(CSpire* pSpire)
{
	m_hSpire = pSpire;
}

CSpire* CStructure::GetSpire() const
{
	return m_hSpire;
}

CStructure* CStructure::CreateStructure(structure_type eType, CSPPlayer* pOwner, CSpire* pSpire, const CScalableVector& vecOrigin)
{
	CStructure* pStructure = nullptr;
	switch (eType)
	{
	default:
	case STRUCTURE_NONE:
		TAssert(false);
		return nullptr;

	case STRUCTURE_SPIRE:
		pStructure = GameServer()->Create<CSpire>("CSpire");
		break;

	case STRUCTURE_MINE:
		pStructure = GameServer()->Create<CMine>("CMine");
		break;
	}

	pStructure->SetOwner(pOwner);
	pStructure->SetSpire(pSpire);
	pStructure->SetMoveParent(pOwner->GetPlayerCharacter()->GetMoveParent());
	pStructure->SetLocalOrigin(vecOrigin);
	if (pStructure->GetMoveParent())
	{
		Vector vecUp = pStructure->GetLocalOrigin().Normalized();
		Matrix4x4 mDirection;
		mDirection.SetUpVector(vecUp);
		mDirection.SetForwardVector(vecUp.Cross(pStructure->GetLocalTransform().GetRightVector()).Normalized());
		mDirection.SetRightVector(mDirection.GetForwardVector().Cross(vecUp).Normalized());
		pStructure->SetLocalAngles(mDirection.GetAngles());
	}

	pStructure->PostConstruction();

	return pStructure;
}
