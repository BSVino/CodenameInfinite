#include "sp_window.h"

#include <time.h>

#include <GL/glew.h>
#include <IL/il.h>
#include <IL/ilu.h>

#include <mtrand.h>

#include <tinker/keys.h>
#include <game/gameserver.h>
#include <game/game.h>
#include <glgui/glgui.h>
#include <renderer/renderer.h>
#include <tinker/cvar.h>

#include "sp_player.h"
#include "sp_character.h"
#include "sp_renderer.h"
#include "planet.h"

CSPWindow::CSPWindow(int argc, char** argv)
	: CGameWindow(argc, argv)
{
	m_pGeneralWindow = NULL;
}

void CSPWindow::SetupEngine()
{
	mtsrand((size_t)time(NULL));

	GameServer()->Initialize();

	glgui::CRootPanel::Get()->SetLighting(false);
	glgui::CRootPanel::Get()->Layout();

	SetupSP();

	GameServer()->SetLoading(false);

	CApplication::Get()->SetMouseCursorEnabled(false);
}

void CSPWindow::SetupSP()
{
	CSPPlayer* pPlayer = GameServer()->Create<CSPPlayer>("CSPPlayer");
	Game()->AddPlayer(pPlayer);

	CSPCharacter* pCharacter = GameServer()->Create<CSPCharacter>("CSPCharacter");
	pCharacter->SetGlobalOrigin(Vector(0, 0, 0));
	pCharacter->SetGlobalAngles(EAngle(10, 80, 90));
	pPlayer->SetCharacter(pCharacter);

	CPlanet* pPlanet = GameServer()->Create<CPlanet>("CPlanet");
	pPlanet->SetGlobalOrigin(Vector(500, 0, 500));
	pPlanet->SetRadius(6378.100f);			// Radius of Earth, 6378.1 km
	pPlanet->SetAtmosphereThickness(50);	// Atmosphere of Earth, about 50km until the end of the stratosphere
	pPlanet->SetMinutesPerRevolution(30);

	pPlanet = GameServer()->Create<CPlanet>("CPlanet");
	pPlanet->SetGlobalOrigin(Vector(10000, 0, 10000));
	pPlanet->SetRadius(3397.0f);			// Radius of Mars, 3397 km
	pPlanet->SetAtmosphereThickness(25);
	pPlanet->SetMinutesPerRevolution(20);

	pCharacter->StandOnNearestPlanet();
}

void CSPWindow::RenderLoading()
{
	glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);

	SwapBuffers();
}

void CSPWindow::DoKeyPress(int c)
{
	BaseClass::DoKeyPress(c);
}

CSPRenderer* CSPWindow::GetRenderer()
{
	return static_cast<CSPRenderer*>(GameServer()->GetRenderer());
}
