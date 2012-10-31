#include "structure.h"

#include <game/gameserver.h>

#include <tengine/renderer/game_renderer.h>

#include "entities/sp_player.h"
#include "entities/sp_playercharacter.h"
#include "planet/planet.h"
#include "planet/terrain_chunks.h"
#include "spire.h"
#include "mine.h"
#include "pallet.h"
#include "ui/command_menu.h"

REGISTER_ENTITY(CStructure);

NETVAR_TABLE_BEGIN(CStructure);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CStructure);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CSPPlayer, m_hOwner);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CSpire, m_hSpire);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CStructure);
INPUTS_TABLE_END();

void CStructure::Spawn()
{
	m_bTakeDamage = true;

	BaseClass::Spawn();

	SetTotalHealth(50);
}

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

void CStructure::Think()
{
	BaseClass::Think();

	if (GameData().GetCommandMenu())
	{
		if (GameData().GetCommandMenu()->WantsToClose())
			GameData().CloseCommandMenu();
		else
			GameData().GetCommandMenu()->Think();
	}
}

void CStructure::PostRender() const
{
	BaseClass::PostRender();

	if (!GameServer()->GetRenderer()->IsRenderingTransparent())
		return;

	if (GameData().GetCommandMenu())
		GameData().GetCommandMenu()->Render();
}

void CStructure::OnUse(CBaseEntity* pUser)
{
	CSPCharacter* pCharacter = dynamic_cast<CSPCharacter*>(pUser);
	if (pCharacter)
		PerformStructureTask(pCharacter);
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

	pStructure->GameData().SetPlayerOwner(pOwner->GetPlayerCharacter()->GameData().GetPlayerOwner());
	pStructure->GameData().SetPlanet(pOwner->GetPlayerCharacter()->GameData().GetPlanet());
	pStructure->SetOwner(pOwner);
	pStructure->SetSpire(pSpire);
	pStructure->SetGlobalOrigin(pOwner->GetPlayerCharacter()->GameData().GetPlanet()->GetGlobalOrigin());     // Avoid floating point precision problems
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

	return GameData().GetGroupTransform();
}

void CStructure::SetPhysicsTransform(const Matrix4x4& m)
{
	CPlanet* pPlanet = GameData().GetPlanet();
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

void CStructure::PostSetLocalTransform(const TMatrix& m)
{
	BaseClass::PostSetLocalTransform(m);

	CPlanet* pPlanet = GameData().GetPlanet();
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
