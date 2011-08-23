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
#include "ui/hud.h"

CSPWindow::CSPWindow(int argc, char** argv)
	: CGameWindow(argc, argv)
{
}

void CSPWindow::SetupEngine()
{
	mtsrand((size_t)time(NULL));

	GameServer()->Initialize();

	glgui::CRootPanel::Get()->AddControl(m_pSPHUD = new CSPHUD());

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
	pCharacter->SetGlobalScalableOrigin(CScalableVector());
	pCharacter->SetGlobalScalableAngles(EAngle(10, 80, 90));
	pPlayer->SetCharacter(pCharacter);

	CPlanet* pPlanet = GameServer()->Create<CPlanet>("CPlanet");
	pPlanet->SetPlanetName("Earth");
	pPlanet->SetGlobalScalableOrigin(CScalableVector(Vector(-7, 0, 7), SCALE_MEGAMETER));
	pPlanet->SetRadius(CScalableFloat(6.3781f, SCALE_MEGAMETER));			// Radius of Earth, 6378.1 km
	pPlanet->SetAtmosphereThickness(CScalableFloat(50.0f, SCALE_KILOMETER));	// Atmosphere of Earth, about 50km until the end of the stratosphere
	pPlanet->SetMinutesPerRevolution(30);
	pPlanet->SetAtmosphereColor(Color(0.25f, 0.41f, 0.64f));

	pPlanet = GameServer()->Create<CPlanet>("CPlanet");
	pPlanet->SetPlanetName("Mars");
	pPlanet->SetGlobalScalableOrigin(CScalableVector(Vector(100, 0, 100), SCALE_GIGAMETER));	// 200Gm, the average distance to Mars
	pPlanet->SetRadius(CScalableFloat(3.397f, SCALE_MEGAMETER));			// Radius of Mars, 3397 km
	pPlanet->SetAtmosphereThickness(CScalableFloat(25.0f, SCALE_KILOMETER));
	pPlanet->SetMinutesPerRevolution(20);
	pPlanet->SetAtmosphereColor(Color(0.64f, 0.25f, 0.25f));

	CStar* pStar = GameServer()->Create<CStar>("CStar");
	pStar->SetGlobalScalableOrigin(CScalableVector(Vector(150, 0, 0), SCALE_GIGAMETER));	// 150Gm, or one AU, the distance to the Sun.
	pStar->SetRadius(CScalableFloat(10.0f, SCALE_GIGAMETER));
	pStar->SetLightColor(Color(255, 242, 143));

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
