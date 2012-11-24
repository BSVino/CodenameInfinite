#include "structure.h"

#include <tinker/cvar.h>

#include <game/gameserver.h>
#include <tengine/renderer/game_renderer.h>

#include "entities/sp_player.h"
#include "entities/sp_playercharacter.h"
#include "planet/planet.h"
#include "planet/terrain_chunks.h"
#include "ui/command_menu.h"

#include "spire.h"
#include "mine.h"
#include "pallet.h"
#include "stove.h"
#include "powercord.h"

REGISTER_ENTITY(CStructure);

NETVAR_TABLE_BEGIN(CStructure);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CStructure);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CSPPlayer, m_hOwner);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CSpire, m_hSpire);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CPowerCord, m_hPowerCord);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, structure_type, m_eStructureType);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, int, m_iTurnsToConstruct);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, float, m_flConstructionTurnTime);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, size_t, m_iBatteryLevel);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, size_t, m_iMaxBatteryLevel);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CStructure);
INPUTS_TABLE_END();

void CStructure::Spawn()
{
	m_bTakeDamage = true;

	m_flConstructionTurnTime = 0;

	m_eStructureType = StructureType();
	TAssert(m_eStructureType);

	GameData().SetCommandMenuParameters(Vector(0, 1, 0), 0.5f, 1.0f);

	BaseClass::Spawn();

	SetTotalHealth(50);

	m_iBatteryLevel = 0;
	m_iMaxBatteryLevel = MaxBatteryLevel();
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

	if (pSpire)
		pSpire->AddUnit(this);
}

CSpire* CStructure::GetSpire() const
{
	return m_hSpire;
}

CVar build_time_construct("build_time_construct", "6");

void CStructure::Think()
{
	BaseClass::Think();

	if (GameData().GetCommandMenu())
	{
		if (IsUnderConstruction() && IsWorkingConstructionTurn())
		{
			CCommandMenu* pMenu = GameData().GetCommandMenu();
			pMenu->SetProgressBar(GameServer()->GetGameTime() - m_flConstructionTurnTime, build_time_construct.GetFloat());
		}

		if (GameData().GetCommandMenu()->WantsToClose())
			GameData().CloseCommandMenu();
		else
			GameData().GetCommandMenu()->Think();
	}

	if (GameServer()->GetGameTime() > m_flConstructionTurnTime + build_time_construct.GetFloat())
		ConstructionTurn();

	if (IsUnderConstruction())
	{
		if (CanAutoOpenMenu())
		{
			// If the player is nearby, looking at me, and not already looking at another command menu, open a temp command menu showing construction info.
			CCommandMenu* pMenu = GameData().CreateCommandMenu(GetOwner()->GetPlayerCharacter());
			SetupMenuButtons();
		}
		else if (CanAutoCloseMenu())
		{
			// If the player goes away or stops looking, close it.
			GameData().CloseCommandMenu();
		}

		if (GameData().GetCommandMenu())
			GameData().GetCommandMenu()->Think();
	}
	else if (TakesPower() && GetOwner() && GetOwner()->GetPlayerCharacter() && GetOwner()->GetPlayerCharacter()->IsHoldingPowerCord())
	{
		if (CanAutoOpenMenu())
		{
			// If the player is nearby, looking at me, and not already looking at another command menu, open a temp command menu showing construction info.
			CCommandMenu* pMenu = GameData().CreateCommandMenu(GetOwner()->GetPlayerCharacter());
			SetupMenuButtons();
		}
		else if (CanAutoCloseMenu())
		{
			// If the player goes away or stops looking, close it.
			GameData().CloseCommandMenu();
		}

		if (GameData().GetCommandMenu())
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

bool CStructure::CanAutoOpenMenu() const
{
	CPlayerCharacter* pOwner = GetOwner()->GetPlayerCharacter();

	if (!pOwner)
		return false;

	if (GetOwner()->IsInBlockPlaceMode())
		return false;

	if (GetOwner()->IsInBlockDesignateMode())
		return false;

	if (GetOwner()->IsInConstructionMode())
		return false;

	TVector vecPlayerEyes = pOwner->GetLocalOrigin() + pOwner->EyeHeight() * DoubleVector(pOwner->GetLocalUpVector());
	TVector vecOwnerProjectionPoint = GetLocalOrigin() + GetLocalTransform().TransformVector(GameData().GetCommandMenuRenderOffset());

	Vector vecToPlayerEyes = (vecPlayerEyes - vecOwnerProjectionPoint).GetMeters();
	float flDistanceToPlayerEyes = vecToPlayerEyes.Length();
	Vector vecProjectionDirection = vecToPlayerEyes/flDistanceToPlayerEyes;

	float flViewAngleDot = AngleVector(pOwner->GetViewAngles()).Dot(vecProjectionDirection);

	CSPPlayer* pPlayerOwner = static_cast<CSPPlayer*>(pOwner->GetControllingPlayer());

	if (!GameData().GetCommandMenu() && flDistanceToPlayerEyes < 4 && flViewAngleDot < -0.8 && !pPlayerOwner->GetActiveCommandMenu())
		return true;

	return false;
}

bool CStructure::CanAutoCloseMenu() const
{
	CPlayerCharacter* pOwner = GetOwner()->GetPlayerCharacter();

	if (!pOwner)
		return false;

	if (GetOwner()->IsInBlockPlaceMode())
		return true;

	if (GetOwner()->IsInBlockDesignateMode())
		return true;

	if (GetOwner()->IsInConstructionMode())
		return true;

	TVector vecPlayerEyes = pOwner->GetLocalOrigin() + pOwner->EyeHeight() * DoubleVector(pOwner->GetLocalUpVector());
	TVector vecOwnerProjectionPoint = GetLocalOrigin() + GetLocalTransform().TransformVector(GameData().GetCommandMenuRenderOffset());

	Vector vecToPlayerEyes = (vecPlayerEyes - vecOwnerProjectionPoint).GetMeters();
	float flDistanceToPlayerEyes = vecToPlayerEyes.Length();
	Vector vecProjectionDirection = vecToPlayerEyes/flDistanceToPlayerEyes;

	float flViewAngleDot = AngleVector(pOwner->GetViewAngles()).Dot(vecProjectionDirection);

	if (GameData().GetCommandMenu() && (GameData().GetCommandMenu()->WantsToClose() || flDistanceToPlayerEyes > 5 || flViewAngleDot > -0.7))
		return true;

	return false;
}

void CStructure::SetTurnsToConstruct(int iTurns)
{
	m_iTurnsToConstruct = iTurns;
	m_iTotalTurnsToConstruct = iTurns;
}

void CStructure::ConstructionTurn()
{
	if (!m_flConstructionTurnTime)
		return;

	m_flConstructionTurnTime = 0;

	if (m_iTurnsToConstruct <= 0)
		return;

	m_iTurnsToConstruct--;

	if (GameData().GetCommandMenu())
		SetupMenuButtons();

	if (!m_iTurnsToConstruct)
		FinishConstruction();
}

void CStructure::FinishConstruction()
{
	GameData().CloseCommandMenu();

	PostConstructionFinished();
}

void CStructure::OnUse(CBaseEntity* pUser)
{
	CSPCharacter* pCharacter = dynamic_cast<CSPCharacter*>(pUser);
	if (pCharacter)
		PerformStructureTask(pCharacter);
}

void CStructure::SetupMenuButtons()
{
	CCommandMenu* pMenu = GameData().GetCommandMenu();

	if (!pMenu)
		return;

	if (IsUnderConstruction())
	{
		pMenu->SetTitle(GetStructureName(m_eStructureType));
		pMenu->SetSubtitle(sprintf("UNDER CONSTRUCTION - %d/%d", m_iTotalTurnsToConstruct-m_iTurnsToConstruct, m_iTotalTurnsToConstruct));

		if (IsWorkingConstructionTurn())
			pMenu->SetProgressBar(GameServer()->GetGameTime() - m_flConstructionTurnTime, build_time_construct.GetFloat());
		else
			pMenu->DisableProgressBar();
	}
	else if (GetOwner() && GetOwner()->GetPlayerCharacter() && GetOwner()->GetPlayerCharacter()->IsHoldingPowerCord())
	{
		pMenu->SetTitle(GetStructureName(m_eStructureType));
		pMenu->SetSubtitle("Connect power?");
	}
}

void CStructure::PerformStructureTask(class CSPCharacter* pCharacter)
{
	if (IsUnderConstruction() && !m_flConstructionTurnTime)
	{
		if (pCharacter->GetControllingPlayer())
			pCharacter->GetControllingPlayer()->Instructor_LessonLearned("develop-structures");

		m_flConstructionTurnTime = GameServer()->GetGameTime();

		CPlayer* pPlayer = pCharacter->GetControllingPlayer();
		if (pPlayer)
		{
			CSPPlayer* pSPPlayer = static_cast<CSPPlayer*>(pPlayer);

			if (!pSPPlayer->GetActiveCommandMenu())
				GameData().CreateCommandMenu(pSPPlayer->GetPlayerCharacter());

			// If it's a player who's building this structure then close any other command menus and activate this one.
			if (pSPPlayer->GetActiveCommandMenu()->GetOwner() != this)
			{
				pSPPlayer->GetActiveCommandMenu()->GetOwner()->GameData().CloseCommandMenu();
				GameData().CreateCommandMenu(pSPPlayer->GetPlayerCharacter());
			}
		}

		if (GameData().GetCommandMenu())
			SetupMenuButtons();
	}

	if (!IsUnderConstruction())
	{
		if (pCharacter->GetControllingPlayer())
			pCharacter->GetControllingPlayer()->Instructor_LessonLearned("interact-with-structures");
	}
}

bool CStructure::IsOccupied() const
{
	if (m_flConstructionTurnTime)
		return true;

	return false;
}

void CStructure::SetPowerCord(CPowerCord* pCord)
{
	if (m_hPowerCord)
		m_hPowerCord->Delete();

	m_hPowerCord = pCord;

	if (GameData().GetCommandMenu())
		GameData().CloseCommandMenu();
}

CStructure* CStructure::GetPowerSource() const
{
	if (!m_hPowerCord)
		return nullptr;

	return m_hPowerCord->GetSource();
}

bool CStructure::DrawPower(size_t iPower)
{
	if (!iPower)
		return false;

	if (iPower > m_iBatteryLevel)
		return false;

	m_iBatteryLevel -= iPower;

	OnPowerDrawn();

	return true;
}

void CStructure::AddPower(size_t iPower)
{
	m_iBatteryLevel = std::min(m_iBatteryLevel + iPower, m_iMaxBatteryLevel);
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

	case STRUCTURE_STOVE:
		pStructure = GameServer()->Create<CStove>("CStove");
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

	GameData().SetGroupTransform(pPlanet->GetChunkManager()->GetPlanetToGroupCenterTransform() * mLocalTransform.GetMeters());
}

static const char* g_szStructureNames[] = {
	"",
	"Spire",
	"Mine",
	"Pallet",
	"Furnace",
};

const char* GetStructureName(structure_type eStructure)
{
	if (eStructure < 0)
		return "";

	if (eStructure >= ITEM_TOTAL)
		return "";

	return g_szStructureNames[eStructure];
}
