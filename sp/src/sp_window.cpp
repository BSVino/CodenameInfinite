#include "sp_window.h"

#include <time.h>

#include <mtrand.h>

#include <tinker/keys.h>
#include <game/gameserver.h>
#include <game/entities/game.h>
#include <glgui/glgui.h>
#include <renderer/renderer.h>
#include <tinker/cvar.h>
#include <game/entities/camera.h>
#include <glgui/rootpanel.h>
#include <renderer/renderingcontext.h>

#include "ui/hud.h"
#include "sp_renderer.h"

CSPWindow::CSPWindow(int argc, char** argv)
	: CGameWindow(argc, argv)
{
}

CRenderer* CSPWindow::CreateRenderer()
{
	return new CSPRenderer();
}

void CSPWindow::SetupEngine()
{
	mtsrand((size_t)time(NULL));

	GameServer()->Initialize();

	glgui::CRootPanel::Get()->SetLighting(false);
	glgui::CRootPanel::Get()->Layout();

	GameServer()->SetLoading(false);

	CApplication::Get()->SetMouseCursorEnabled(false);
}

void CSPWindow::RenderLoading()
{
	CRenderingContext c(GetRenderer());
	c.ClearDepth();
	c.ClearColor();

	SwapBuffers();
}

CSPRenderer* CSPWindow::GetRenderer()
{
	return static_cast<CSPRenderer*>(GameServer()->GetRenderer());
}

CSPHUD* CSPWindow::GetHUD()
{
	return static_cast<CSPHUD*>(m_pHUD);
}
