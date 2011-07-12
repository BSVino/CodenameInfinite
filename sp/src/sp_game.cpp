#include "sp_game.h"

#include <tinker/application.h>
#include <renderer/renderer.h>
#include <game/level.h>

#include "sp_player.h"
#include "sp_character.h"
#include "sp_camera.h"
#include "sp_renderer.h"

CGame* CreateGame()
{
	return GameServer()->Create<CSPGame>("CSPGame");
}

CRenderer* CreateRenderer()
{
	return new CSPRenderer();
}

CCamera* CreateCamera()
{
	CCamera* pCamera = new CSPCamera();
	return pCamera;
}

CLevel* CreateLevel()
{
	return new CLevel();
}

REGISTER_ENTITY(CSPGame);

NETVAR_TABLE_BEGIN(CSPGame);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CSPGame);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CSPGame);
INPUTS_TABLE_END();

void CSPGame::Precache()
{
}

void CSPGame::Think()
{
	BaseClass::Think();
}

CSPCharacter* CSPGame::GetLocalPlayerCharacter()
{
	return static_cast<CSPCharacter*>(GetLocalPlayer()->GetCharacter());
}
