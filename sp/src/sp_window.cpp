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
#include <game/camera.h>

#include "sp_player.h"
#include "sp_character.h"
#include "sp_renderer.h"
#include "planet.h"
#include "star.h"

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
	pPlanet->SetGlobalOrigin(Vector(7, 0, 7));
	pPlanet->SetRadius(CScalableFloat(6.3781f, SCALE_MEGAMETER));			// Radius of Earth, 6378.1 km
	pPlanet->SetAtmosphereThickness(CScalableFloat(50, SCALE_KILOMETER));	// Atmosphere of Earth, about 50km until the end of the stratosphere
	pPlanet->SetMinutesPerRevolution(30);

	pPlanet = GameServer()->Create<CPlanet>("CPlanet");
	pPlanet->SetGlobalOrigin(Vector(-5, 0, -5));
	pPlanet->SetRadius(CScalableFloat(3.397f, SCALE_MEGAMETER));			// Radius of Mars, 3397 km
	pPlanet->SetAtmosphereThickness(CScalableFloat(25, SCALE_KILOMETER));
	pPlanet->SetMinutesPerRevolution(20);

	CStar* pStar = GameServer()->Create<CStar>("CStar");
	pStar->SetGlobalOrigin(Vector(-3000, 0, 3000));
	pStar->SetRadius(CScalableFloat(695.5f, SCALE_MEGAMETER));				// Radius of the sun, 695500km

	//pCharacter->StandOnNearestPlanet();
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
