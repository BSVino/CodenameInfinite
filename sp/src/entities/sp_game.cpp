#include "sp_game.h"

#include <tinker/application.h>
#include <renderer/renderer.h>
#include <game/level.h>
#include <tinker/cvar.h>
#include <tengine/ui/gamewindow.h>
#include <tengine/ui/instructor.h>

#include "entities/sp_player.h"
#include "entities/sp_playercharacter.h"
#include "entities/sp_character.h"
#include "entities/sp_camera.h"
#include "sp_renderer.h"
#include "planet/planet.h"
#include "entities/star.h"
#include "ui/hud.h"
#include "planet/terrain_chunks.h"
#include "planet/terrain_lumps.h"
#include "entities/bots/helper.h"

CGame* CreateGame()
{
	return GameServer()->Create<CSPGame>("CSPGame");
}

CResource<CLevel> CreateLevel()
{
	return CResource<CLevel>(new CLevel());
}

CHUDViewport* CreateHUD()
{
	return new CSPHUD();
}

tstring GetInitialGameMode()
{
	return "game";
}

REGISTER_ENTITY(CSPGame);

NETVAR_TABLE_BEGIN(CSPGame);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CSPGame);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CSPGame);
INPUTS_TABLE_END();

CSPGame::CSPGame()
{
	m_bWaitingForTerrainToGenerate = false;
}

void CSPGame::Precache()
{
}

void CSPGame::SetupGame(tstring sType)
{
	if (sType == "game")
	{
		CSPPlayer* pPlayer = GameServer()->Create<CSPPlayer>("CSPPlayer");
		Game()->AddPlayer(pPlayer);

		CPlayerCharacter* pCharacter = GameServer()->Create<CPlayerCharacter>("CPlayerCharacter");
		if (pCharacter->GetHelperBot())
			pCharacter->GetHelperBot()->GameData().SetPlayerOwner(pPlayer);
		pCharacter->SetGlobalOrigin(CScalableVector(Vector(0, 0, 0), SCALE_MEGAMETER));
		pCharacter->SetViewAngles(EAngle(0, 140, 0));
		pCharacter->GameData().SetPlayerOwner(pPlayer);
		pPlayer->SetCharacter(pCharacter);

		CStar* pStar = GameServer()->Create<CStar>("CStar");
		pStar->SetGlobalOrigin(CScalableVector(Vector(150, 0, 0), SCALE_GIGAMETER));	// 150Gm, or one AU, the distance to the Sun.
		pStar->SetRadius(CScalableFloat(10.0f, SCALE_GIGAMETER));
		pStar->SetLightColor(Color(255, 242, 143));

		CPlanet* pPlanet = GameServer()->Create<CPlanet>("CPlanet");
		pPlanet->SetStar(pStar);
		pPlanet->SetPlanetName("Earth");
		pPlanet->SetGlobalOrigin(CScalableVector(Vector(-7, 0, 7), SCALE_MEGAMETER));
		pPlanet->SetRadius(CScalableFloat(6.3781f, SCALE_MEGAMETER));			// Radius of Earth, 6378.1 km
		pPlanet->SetAtmosphereThickness(CScalableFloat(50.0f, SCALE_KILOMETER));	// Atmosphere of Earth, about 50km until the end of the stratosphere
		pPlanet->SetMinutesPerRevolution(30);
		pPlanet->SetAtmosphereColor(Color(0.25f, 0.41f, 0.64f));
		pPlanet->SetRandomSeed(0);

		pPlanet = GameServer()->Create<CPlanet>("CPlanet");
		pPlanet->SetStar(pStar);
		pPlanet->SetPlanetName("Moon");
		pPlanet->SetGlobalOrigin(CScalableVector(Vector(-7, 0, 7), SCALE_MEGAMETER) - CScalableVector(Vector(0, 0, 100), SCALE_MEGAMETER));
		pPlanet->SetRadius(CScalableFloat(1.73f, SCALE_MEGAMETER));
		pPlanet->SetAtmosphereThickness(CScalableFloat(5.0f, SCALE_KILOMETER));
		pPlanet->SetMinutesPerRevolution(0);
		pPlanet->SetAtmosphereColor(Color(0.25f, 0.25f, 0.3f));
		pPlanet->SetRandomSeed(3);

		pPlanet = GameServer()->Create<CPlanet>("CPlanet");
		pPlanet->SetStar(pStar);
		pPlanet->SetPlanetName("Mars");
		pPlanet->SetGlobalOrigin(CScalableVector(Vector(100, 0, 100), SCALE_GIGAMETER));	// 200Gm, the average distance to Mars
		pPlanet->SetRadius(CScalableFloat(3.397f, SCALE_MEGAMETER));			// Radius of Mars, 3397 km
		pPlanet->SetAtmosphereThickness(CScalableFloat(35.0f, SCALE_KILOMETER));
		pPlanet->SetMinutesPerRevolution(20);
		pPlanet->SetAtmosphereColor(Color(0.64f, 0.25f, 0.25f));
		pPlanet->SetRandomSeed(1);

		TVector vecProxima(Vector(30000, 0, 20000), SCALE_TERAMETER);

		pStar = GameServer()->Create<CStar>("CStar");
		pStar->SetGlobalOrigin(vecProxima);
		pStar->SetRadius(CScalableFloat(10.0f, SCALE_GIGAMETER));
		pStar->SetLightColor(Color(255, 242, 143));

		pPlanet = GameServer()->Create<CPlanet>("CPlanet");
		pPlanet->SetStar(pStar);
		pPlanet->SetPlanetName("Proxima");
		pPlanet->SetGlobalOrigin(vecProxima + CScalableVector(Vector(100, 0, 100), SCALE_GIGAMETER));
		pPlanet->SetRadius(CScalableFloat(5.0f, SCALE_MEGAMETER));
		pPlanet->SetAtmosphereThickness(CScalableFloat(45.0f, SCALE_KILOMETER));
		pPlanet->SetMinutesPerRevolution(10);
		pPlanet->SetAtmosphereColor(Color(0.31f, 0.64f, 0.25f));
		pPlanet->SetRandomSeed(2);

		pCharacter->StandOnNearestPlanet();

		Application()->SetMouseCursorEnabled(false);

		GameServer()->AddAllToPrecacheList();

		// Pause it until our terrain is done generating.
		GameServer()->SetTimeScale(0);
		m_bWaitingForTerrainToGenerate = true;
	}
	else if (sType == "menu")
	{
		Application()->SetMouseCursorEnabled(true);
	}
	else
	{
		TError("Unrecognized game type: " + sType + "\n");
	}
}

void CSPGame::Think()
{
	BaseClass::Think();

	if (m_bWaitingForTerrainToGenerate)
	{
		TAssert(GetLocalSPPlayer() && GetLocalSPPlayer()->GetPlayerCharacter());
		CPlayerCharacter* pCharacter = GetLocalSPPlayer()->GetPlayerCharacter();

		if (!pCharacter->GetNearestPlanet())
		{
			TUnimplemented(); // Should be just fine but I haven't tried it.
			m_bWaitingForTerrainToGenerate = false;
			GameServer()->SetTimeScale(1);
		}

		if (pCharacter && pCharacter->GetNearestPlanet())
		{
			CPlanet* pPlanet = pCharacter->GetNearestPlanet();
			if (!pPlanet->GetChunkManager()->IsGenerating() && !pPlanet->GetLumpManager()->IsGenerating())
			{
				m_bWaitingForTerrainToGenerate = false;
				GameServer()->SetTimeScale(1);
			}
		}
	}
}

bool CSPGame::TakesDamageFrom(CBaseEntity* pVictim, CBaseEntity* pAttacker)
{
	if (pVictim->GameData().GetPlayerOwner() == pAttacker->GameData().GetPlayerOwner())
		return false;

	return true;
}

CVar sv_freestructures("sv_freestructures", "off");

size_t CSPGame::StructureCost(structure_type eStructure, item_t eItem) const
{
#ifdef _DEBUG
	if (sv_freestructures.GetBool())
		return 0;
#endif

	switch (eStructure)
	{
	case STRUCTURE_MINE:
		if (eItem == ITEM_WOOD)
			return 3;
		else
			return 0;

	case STRUCTURE_PALLET:
		if (eItem == ITEM_STONE)
			return 2;
		else
			return 0;

	case STRUCTURE_SPIRE:
		if (eItem == ITEM_WOOD)
			return 5;
		else if (eItem == ITEM_STONE)
			return 8;
		else
			return 0;

	case STRUCTURE_STOVE:
		if (eItem == ITEM_STONE)
			return 3;
		else if (eItem == ITEM_DIRT)
			return 5;
		else
			return 0;

	default:
		TAssert(false);
		return 0;
	}
}

extern bool ValidPlayerAliveConditions(CPlayer *pPlayer, class CLesson *pLesson);

bool ValidPlayerNotFlyingConditions( CPlayer *pPlayer, class CLesson *pLesson )
{
	if (!ValidPlayerAliveConditions(pPlayer, pLesson))
		return false;

	CSPPlayer* pSPPlayer = static_cast<CSPPlayer*>(pPlayer);
	CPlayerCharacter* pCharacter = pSPPlayer->GetPlayerCharacter();

	TAssert(pCharacter);
	if (!pCharacter)
		return false;

	if (pCharacter->IsFlying())
		return false;

	return true;
}

bool ValidPlayerFlyingConditions( CPlayer *pPlayer, class CLesson *pLesson )
{
	if (!ValidPlayerAliveConditions(pPlayer, pLesson))
		return false;

	CSPPlayer* pSPPlayer = static_cast<CSPPlayer*>(pPlayer);
	CPlayerCharacter* pCharacter = pSPPlayer->GetPlayerCharacter();

	TAssert(pCharacter);
	if (!pCharacter)
		return false;

	if (!pCharacter->IsFlying())
		return false;

	return true;
}

bool ValidStructuresUnderConstructionConditions( CPlayer *pPlayer, class CLesson *pLesson )
{
	if (!ValidPlayerAliveConditions(pPlayer, pLesson))
		return false;

	if (!ValidPlayerNotFlyingConditions(pPlayer, pLesson))
		return false;

	CSPPlayer* pSPPlayer = static_cast<CSPPlayer*>(pPlayer);
	CPlayerCharacter* pCharacter = pSPPlayer->GetPlayerCharacter();

	TAssert(pCharacter);
	if (!pCharacter)
		return false;

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

		// If we're working our construction turn don't worry about it.
		if (pStructure->IsOccupied())
			continue;

		if ((pStructure->GetGlobalOrigin() - pCharacter->GetGlobalOrigin()).LengthSqr() > TFloat(1000).Squared())
			continue;

		return true;
	}

	return false;
}

bool ValidStructuresNotUnderConstructionConditions( CPlayer *pPlayer, class CLesson *pLesson )
{
	if (!ValidPlayerAliveConditions(pPlayer, pLesson))
		return false;

	if (!ValidPlayerNotFlyingConditions(pPlayer, pLesson))
		return false;

	CSPPlayer* pSPPlayer = static_cast<CSPPlayer*>(pPlayer);
	CPlayerCharacter* pCharacter = pSPPlayer->GetPlayerCharacter();

	TAssert(pCharacter);
	if (!pCharacter)
		return false;

	for (size_t i = 0; i < GameServer()->GetMaxEntities(); i++)
	{
		CBaseEntity* pEntity = CBaseEntity::GetEntity(i);
		if (!pEntity)
			continue;

		CStructure* pStructure = dynamic_cast<CStructure*>(pEntity);
		if (!pStructure)
			continue;

		if (pStructure->IsUnderConstruction())
			continue;

		if (pStructure->IsOccupied())
			continue;

		if ((pStructure->GetGlobalOrigin() - pCharacter->GetGlobalOrigin()).LengthSqr() > TFloat(1000).Squared())
			continue;

		return true;
	}

	return false;
}

bool ValidBotsWithoutOrdersConditions( CPlayer *pPlayer, class CLesson *pLesson )
{
	if (!ValidPlayerAliveConditions(pPlayer, pLesson))
		return false;

	if (!ValidPlayerNotFlyingConditions(pPlayer, pLesson))
		return false;

	CSPPlayer* pSPPlayer = static_cast<CSPPlayer*>(pPlayer);
	CPlayerCharacter* pCharacter = pSPPlayer->GetPlayerCharacter();

	TAssert(pCharacter);
	if (!pCharacter)
		return false;

	for (size_t i = 0; i < GameServer()->GetMaxEntities(); i++)
	{
		CBaseEntity* pEntity = CBaseEntity::GetEntity(i);
		if (!pEntity)
			continue;

		CBot* pBot = dynamic_cast<CBot*>(pEntity);
		if (!pBot)
			continue;

		if (pCharacter->GetHelperBot() == pBot)
			continue;

		if (pBot->GetTask() != TASK_NONE)
			continue;

		return true;
	}

	return false;
}

bool ValidTimeOfDayAfterNoonConditions( CPlayer *pPlayer, class CLesson *pLesson )
{
	if (!ValidPlayerAliveConditions(pPlayer, pLesson))
		return false;

	if (!ValidPlayerNotFlyingConditions(pPlayer, pLesson))
		return false;

	CSPPlayer* pSPPlayer = static_cast<CSPPlayer*>(pPlayer);
	CPlayerCharacter* pCharacter = pSPPlayer->GetPlayerCharacter();

	TAssert(pCharacter);
	if (!pCharacter)
		return false;

	CPlanet* pPlanet = pCharacter->GameData().GetPlanet();
	if (!pPlanet)
		return false;

	float flTimeOfDay = pPlanet->GetTimeOfDay(pCharacter->GetGlobalOrigin());

	if (flTimeOfDay > M_PI/2 && flTimeOfDay < M_PI)
		return true;

	return false;
}

bool ValidPlayerInSpaceConditions( CPlayer *pPlayer, class CLesson *pLesson )
{
	if (!ValidPlayerAliveConditions(pPlayer, pLesson))
		return false;

	if (!ValidPlayerFlyingConditions(pPlayer, pLesson))
		return false;

	CSPPlayer* pSPPlayer = static_cast<CSPPlayer*>(pPlayer);
	CPlayerCharacter* pCharacter = pSPPlayer->GetPlayerCharacter();

	TAssert(pCharacter);
	if (!pCharacter)
		return false;

	CPlanet* pPlanet = pCharacter->GameData().GetPlanet();
	if (!pPlanet)
		return true;

	return false;
}

pfnConditionsMet Game_GetInstructorConditions(const tstring& sConditions)
{
	if (sConditions == "ValidPlayerNotFlying")
		return ValidPlayerNotFlyingConditions;
	if (sConditions == "ValidPlayerFlying")
		return ValidPlayerFlyingConditions;
	if (sConditions == "ValidStructuresUnderConstruction")
		return ValidStructuresUnderConstructionConditions;
	if (sConditions == "ValidStructuresNotUnderConstruction")
		return ValidStructuresNotUnderConstructionConditions;
	if (sConditions == "ValidBotsWithoutOrders")
		return ValidBotsWithoutOrdersConditions;
	if (sConditions == "ValidTimeOfDayAfterNoon")
		return ValidTimeOfDayAfterNoonConditions;
	if (sConditions == "ValidPlayerInSpace")
		return ValidPlayerInSpaceConditions;

	return CInstructor::GetBaseConditions(sConditions);
}

CSPPlayer* CSPGame::GetLocalSPPlayer()
{
	return static_cast<CSPPlayer*>(GetLocalPlayer());
}

CPlayerCharacter* CSPGame::GetLocalPlayerCharacter()
{
	return static_cast<CPlayerCharacter*>(GetLocalPlayer()->GetCharacter());
}

CSPRenderer* CSPGame::GetSPRenderer()
{
	return static_cast<CSPRenderer*>(GameServer()->GetRenderer());
}
