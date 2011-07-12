#include "gamewindow.h"

#include <platform.h>

#include <tinker/profiler.h>
#include <game/gameserver.h>
#include <glgui/glgui.h>
#include <game/camera.h>
#include <renderer/renderer.h>
#include <tengine/game/game.h>

CGameWindow::CGameWindow(int argc, char** argv)
	: CApplication(argc, argv)
{
	m_bHaveLastMouse = false;
}

void CGameWindow::OpenWindow()
{
	int iScreenWidth, iScreenHeight;
	GetScreenSize(iScreenWidth, iScreenHeight);

#ifdef _DEBUG
	BaseClass::OpenWindow(iScreenWidth/2, iScreenHeight/2, false, false);
#else
	BaseClass::OpenWindow(iScreenWidth, iScreenHeight, true, false);
#endif

	RenderLoading();

	m_pGameServer = new CGameServer();

	m_pRenderer = CreateRenderer();
	m_pRenderer->Initialize();
}

CGameWindow::~CGameWindow()
{
	if (m_pRenderer)
		delete m_pRenderer;

	if (m_pGameServer)
		delete m_pGameServer;
}

void CGameWindow::Run()
{
	while (IsOpen())
	{
		CProfiler::BeginFrame();

		if (true)
		{
			TPROF("CGameWindow::Run");

			PreFrame();

			if (GameServer()->IsHalting())
			{
				//DestroyGame();
				//CreateGame(m_eRestartAction);
			}

			float flTime = GetTime();
			if (GameServer())
			{
				if (GameServer()->IsLoading())
				{
					// Pump the network
					CNetwork::Think();
					RenderLoading();
					continue;
				}
				else if (GameServer()->IsClient() && !GameNetwork()->IsConnected())
				{
					//DestroyGame();
					//CreateGame(GAMETYPE_MENU);
				}
				else
				{
					GameServer()->Think(flTime);
					Render();
				}
			}
		}

		CProfiler::Render();
		SwapBuffers();
	}
}

void CGameWindow::Render()
{
	if (!GameServer())
		return;

	TPROF("CGameWindow::Render");

	GameServer()->Render();

	if (true)
	{
		TPROF("GUI");
		glgui::CRootPanel::Get()->Think(GameServer()->GetGameTime());
		glgui::CRootPanel::Get()->Paint(0, 0, (int)m_iWindowWidth, (int)m_iWindowHeight);
	}
}

void CGameWindow::KeyPress(int c)
{
	BaseClass::KeyPress(c);

	if (GameServer() && GameServer()->GetCamera())
		GameServer()->GetCamera()->KeyDown(c);

	if (Game())
	{
		for (size_t i = 0; i < Game()->GetNumLocalPlayers(); i++)
		{
			CPlayer* pPlayer = Game()->GetLocalPlayer(i);
			pPlayer->KeyPress(c);
		}
	}
}

void CGameWindow::KeyRelease(int c)
{
	BaseClass::KeyRelease(c);

	if (GameServer() && GameServer()->GetCamera())
		GameServer()->GetCamera()->KeyUp(c);

	if (Game())
	{
		for (size_t i = 0; i < Game()->GetNumLocalPlayers(); i++)
		{
			CPlayer* pPlayer = Game()->GetLocalPlayer(i);
			pPlayer->KeyRelease(c);
		}
	}
}

void CGameWindow::MouseMotion(int x, int y)
{
	BaseClass::MouseMotion(x, y);

	if (GameServer() && GameServer()->GetCamera())
		GameServer()->GetCamera()->MouseInput(x, y);

	if (Game() && m_bHaveLastMouse)
	{
		int dx = x - m_iLastMouseX;
		int dy = y - m_iLastMouseY;

		for (size_t i = 0; i < Game()->GetNumLocalPlayers(); i++)
		{
			CPlayer* pPlayer = Game()->GetLocalPlayer(i);
			pPlayer->MouseMotion(dx, dy);
		}
	}

	m_bHaveLastMouse = true;
	m_iLastMouseX = x;
	m_iLastMouseY = y;
}

void CGameWindow::MouseInput(int iButton, int iState)
{
	BaseClass::MouseInput(iButton, iState);

	if (GameServer() && GameServer()->GetCamera())
		GameServer()->GetCamera()->MouseButton(iButton, iState);
}
