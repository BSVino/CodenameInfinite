#include "structure.h"

#include <game/gameserver.h>

#include "entities/sp_player.h"
#include "entities/sp_playercharacter.h"
#include "planet/planet.h"
#include "planet/terrain_chunks.h"
#include "spire.h"
#include "mine.h"
#include "pallet.h"

REGISTER_ENTITY(CStructure);

NETVAR_TABLE_BEGIN(CStructure);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CStructure);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CSPPlayer, m_hOwner);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CSpire, m_hSpire);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CStructure);
INPUTS_TABLE_END();

void CStructure::SetOwner(class CSPPlayer* pOwner)
{
	m_hOwner = pOwner;
}

CSPPlayer* CStructure::GetOwner() const
{
	return m_hOwner;
}

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

	case STRUCTURE_PALLET:
		pStructure = GameServer()->Create<CPallet>("CPallet");
		break;
	}

	pStructure->GameData().SetPlanet(pOwner->GetPlayerCharacter()->GameData().GetPlanet());
	pStructure->SetOwner(pOwner);
	pStructure->SetSpire(pSpire);
	pStructure->SetMoveParent(pOwner->GetPlayerCharacter()->GameData().GetPlanet());
	pStructure->SetLocalOrigin(vecOrigin);
	if (pStructure->GameData().GetPlanet())
	{
		Vector vecUp = pStructure->GetLocalOrigin().Normalized();
		Matrix4x4 mDirection;
		mDirection.SetUpVector(vecUp);
		mDirection.SetForwardVector(vecUp.Cross(pOwner->GetPlayerCharacter()->GetLocalTransform().GetRightVector()).Normalized());
		mDirection.SetRightVector(mDirection.GetForwardVector().Cross(vecUp).Normalized());
		pStructure->SetLocalAngles(mDirection.GetAngles());
	}

	pStructure->AddToPhysics(CT_STATIC_MESH);

	pStructure->PostConstruction();

	return pStructure;
}

const Matrix4x4 CStructure::GetPhysicsTransform() const
{
	CPlanet* pPlanet = GameData().GetPlanet();
	if (!pPlanet)
		return GetLocalTransform();

	if (!pPlanet->GetChunkManager()->HasGroupCenter())
		return GetLocalTransform();

	return pPlanet->GetChunkManager()->GetPlanetToGroupCenterTransform() * Matrix4x4(GetLocalTransform());
}
