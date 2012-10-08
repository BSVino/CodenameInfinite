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
		pCharacter->SetGlobalOrigin(CScalableVector(Vector(0.11f, 0, 0), SCALE_MEGAMETER));
		pCharacter->SetViewAngles(EAngle(0, 140, 0));
		pPlayer->SetCharacter(pCharacter);

		pPlayer->AddSpires(1);

		CPlanet* pPlanet = GameServer()->Create<CPlanet>("CPlanet");
		pPlanet->SetPlanetName("Earth");
		pPlanet->SetGlobalOrigin(CScalableVector(Vector(-7, 0, 7), SCALE_MEGAMETER));
		pPlanet->SetRadius(CScalableFloat(6.3781f, SCALE_MEGAMETER));			// Radius of Earth, 6378.1 km
		pPlanet->SetAtmosphereThickness(CScalableFloat(50.0f, SCALE_KILOMETER));	// Atmosphere of Earth, about 50km until the end of the stratosphere
		pPlanet->SetMinutesPerRevolution(30);
		pPlanet->SetAtmosphereColor(Color(0.25f, 0.41f, 0.64f));
		pPlanet->SetRandomSeed(0);

		pPlanet = GameServer()->Create<CPlanet>("CPlanet");
		pPlanet->SetPlanetName("Mars");
		pPlanet->SetGlobalOrigin(CScalableVector(Vector(100, 0, 100), SCALE_GIGAMETER));	// 200Gm, the average distance to Mars
		pPlanet->SetRadius(CScalableFloat(3.397f, SCALE_MEGAMETER));			// Radius of Mars, 3397 km
		pPlanet->SetAtmosphereThickness(CScalableFloat(25.0f, SCALE_KILOMETER));
		pPlanet->SetMinutesPerRevolution(20);
		pPlanet->SetAtmosphereColor(Color(0.64f, 0.25f, 0.25f));
		pPlanet->SetRandomSeed(1);

		CStar* pStar = GameServer()->Create<CStar>("CStar");
		pStar->SetGlobalOrigin(CScalableVector(Vector(150, 0, 0), SCALE_GIGAMETER));	// 150Gm, or one AU, the distance to the Sun.
		pStar->SetRadius(CScalableFloat(10.0f, SCALE_GIGAMETER));
		pStar->SetLightColor(Color(255, 242, 143));

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

pfnConditionsMet Game_GetInstructorConditions(const tstring& sConditions)
{
	if (sConditions == "ValidPlayerNotFlying")
		return ValidPlayerNotFlyingConditions;
	if (sConditions == "ValidPlayerFlying")
		return ValidPlayerFlyingConditions;

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
