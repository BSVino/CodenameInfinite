#include "digitankswindow.h"

#ifdef _WIN32
#include <Windows.h>
#endif

#include <GL/glew.h>
#include <time.h>

#include <common.h>
#include <vector.h>
#include <strutils.h>

#include <mtrand.h>
#include <configfile.h>
#include <platform.h>
#include <network/network.h>
#include <sound/sound.h>
#include <tinker/cvar.h>
#include <tinker/profiler.h>
#include <tinker/portals/portal.h>
#include <models/texturelibrary.h>
#include <tinker/lobby/lobby_client.h>
#include <tinker/console.h>

#include "glgui/glgui.h"
#include "digitanks/digitanksgame.h"
#include "digitanks/digitankslevel.h"
#include "debugdraw.h"
#include "hud.h"
#include "instructor.h"
#include "game/camera.h"
#include "shaders/shaders.h"
#include "menu.h"
#include "ui.h"
#include "renderer/renderer.h"
#include "digitanks/campaign/campaigndata.h"

ConfigFile c( GetAppDataDirectory(L"Digitanks", L"options.cfg") );

CDigitanksWindow::CDigitanksWindow(int argc, char** argv)
	: CApplication(argc, argv)
{
	m_pGameServer = NULL;
	m_pHUD = NULL;
	m_pInstructor = NULL;
	m_pChatBox = NULL;
	m_pCampaign = NULL;

	m_eRestartAction = GAMETYPE_MENU;

	m_bBoxSelect = false;

	m_iMouseLastX = 0;
	m_iMouseLastY = 0;

	m_flLastClick = 0;

	int iScreenWidth;
	int iScreenHeight;

	GetScreenSize(iScreenWidth, iScreenHeight);

	if (c.isFileValid())
	{
		m_iWindowWidth = c.read<int>(L"width", 1024);
		m_iWindowHeight = c.read<int>(L"height", 768);

		m_bCfgFullscreen = !c.read<bool>(L"windowed", true);
		m_bConstrainMouse = c.read<bool>(L"constrainmouse", true);

		m_bContextualCommands = c.read<bool>(L"contextualcommands", false);
		m_bReverseSpacebar = c.read<bool>(L"reversespacebar", false);

		m_bWantsFramebuffers = c.read<bool>(L"useframebuffers", true);
		m_bWantsShaders = c.read<bool>(L"useshaders", true);

		SetSoundVolume(c.read<float>(L"soundvolume", 0.8f));
		SetMusicVolume(c.read<float>(L"musicvolume", 0.8f));

		m_iInstallID = c.read<int>(L"installid", RandomInt(10000000, 99999999));

		m_sNickname = c.read<eastl::string16>(L"nickname", L"");
	}
	else
	{
		m_iWindowWidth = iScreenWidth*2/3;
		m_iWindowHeight = iScreenHeight*2/3;

		m_bCfgFullscreen = false;

		m_bConstrainMouse = true;

		m_bContextualCommands = false;
		m_bReverseSpacebar = false;

		m_bWantsFramebuffers = true;
		m_bWantsShaders = true;

		SetSoundVolume(0.8f);
		SetMusicVolume(0.8f);

		m_iInstallID = RandomInt(10000000, 99999999);

		m_sNickname = L"";
	}

	if (m_iWindowWidth < 1024)
		m_iWindowWidth = 1024;

	if (m_iWindowHeight < 768)
		m_iWindowHeight = 768;

	if (IsFile(GetAppDataDirectory(L"Digitanks", L"campaign.txt")))
	{
		m_pCampaign = new CCampaignData(CCampaignInfo::GetCampaignInfo());
		m_pCampaign->ReadData(GetAppDataDirectory(L"Digitanks", L"campaign.txt"));
	}

	m_iTotalProgress = 0;
}

void CDigitanksWindow::OpenWindow()
{
	glgui::CLabel::AddFont(L"header", L"fonts/header.ttf");
	glgui::CLabel::AddFont(L"text", L"fonts/text.ttf");
	glgui::CLabel::AddFont(L"smileys", L"fonts/smileys.ttf");
	glgui::CLabel::AddFont(L"cameramissile", L"fonts/cameramissile.ttf");

	BaseClass::OpenWindow(m_iWindowWidth, m_iWindowHeight, m_bCfgFullscreen, false);

	m_iCursors = CTextureLibrary::AddTextureID(L"textures/cursors.png");
	m_iLoading = CTextureLibrary::AddTextureID(L"textures/loading.png");
	m_iLunarWorkshop = CTextureLibrary::AddTextureID(L"textures/lunar-workshop.png");

	RenderLoading();

	InitUI();

	CNetwork::Initialize();

	// Save out the configuration file now that we know this config loads properly.
	SetConfigWindowDimensions(m_iWindowWidth, m_iWindowHeight);

	if (m_sNickname.length() == 0)
	{
		if (TPortal_IsAvailable())
		{
			m_sNickname = TPortal_GetPlayerNickname();
			TMsg(eastl::string16(L"Retrieved player nickname from ") + TPortal_GetPortalIdentifier() + L": " + m_sNickname + L"\n");
		}
		else
			m_sNickname = L"Noobie";
	}

	SaveConfig();
}

CDigitanksWindow::~CDigitanksWindow()
{
	CNetwork::Deinitialize();

	delete m_pMenu;
	delete m_pMainMenu;

	DestroyGame();
}

void CDigitanksWindow::RenderLoading()
{
	glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, m_iWindowWidth, m_iWindowHeight, 0, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glPushAttrib(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_ENABLE_BIT|GL_TEXTURE_BIT);

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glgui::CRootPanel::PaintTexture(m_iLoading, m_iWindowWidth/2 - 150, m_iWindowHeight/2 - 150, 300, 300);
	glgui::CRootPanel::PaintTexture(GetLunarWorkshopLogo(), m_iWindowWidth-200-20, m_iWindowHeight - 200, 200, 200);

	float flWidth = glgui::CLabel::GetTextWidth(m_sAction, m_sAction.length(), L"text", 12);
	glgui::CLabel::PaintText(m_sAction, m_sAction.length(), L"text", 12, (float)m_iWindowWidth/2 - flWidth/2, (float)m_iWindowHeight/2 + 170);

	if (m_iTotalProgress)
	{
		glDisable(GL_TEXTURE_2D);
		float flProgress = (float)m_iProgress/(float)m_iTotalProgress;
		glgui::CBaseControl::PaintRect(m_iWindowWidth/2 - 200, m_iWindowHeight/2 + 190, (int)(400*flProgress), 10, Color(255, 255, 255));
	}

	CApplication::Get()->GetConsole()->Paint();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();   

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glPopAttrib();

	SwapBuffers();
}

void CDigitanksWindow::RenderMouseCursor()
{
	if (m_eMouseCursor == MOUSECURSOR_NONE)
		return;

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, (double)m_iWindowWidth, (double)m_iWindowHeight, 0, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glPushAttrib(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_ENABLE_BIT|GL_TEXTURE_BIT);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glEnable(GL_COLOR_MATERIAL);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glShadeModel(GL_SMOOTH);

	int mx, my;
	GetMousePosition(mx, my);

	if (m_eMouseCursor == MOUSECURSOR_SELECT)
		glgui::CBaseControl::PaintSheet(m_iCursors, mx-20, my-20, 80, 40, 160, 0, 80, 40, 256, 128);
	else if (m_eMouseCursor == MOUSECURSOR_BUILD)
		glgui::CBaseControl::PaintSheet(m_iCursors, mx-20, my-20, 80, 40, 0, 0, 80, 40, 256, 128);
	else if (m_eMouseCursor == MOUSECURSOR_BUILDINVALID)
		glgui::CBaseControl::PaintSheet(m_iCursors, mx-20, my-20, 80, 40, 80, 0, 80, 40, 256, 128);
	else if (m_eMouseCursor == MOUSECURSOR_MOVE)
		glgui::CBaseControl::PaintSheet(m_iCursors, mx-20, my-20, 80, 40, 0, 40, 80, 40, 256, 128);
	else if (m_eMouseCursor == MOUSECURSOR_MOVEAUTO)
		glgui::CBaseControl::PaintSheet(m_iCursors, mx-20, my-20, 80, 40, 80, 40, 80, 40, 256, 128);
	else if (m_eMouseCursor == MOUSECURSOR_ROTATE)
		glgui::CBaseControl::PaintSheet(m_iCursors, mx-20, my-20, 80, 40, 0, 80, 80, 40, 256, 128);
	else if (m_eMouseCursor == MOUSECURSOR_AIM)
		glgui::CBaseControl::PaintSheet(m_iCursors, mx-20, my-20, 80, 40, 160, 40, 80, 40, 256, 128);
	else if (m_eMouseCursor == MOUSECURSOR_AIMENEMY)
	{
		if (Oscillate(GameServer()->GetGameTime(), 0.4f) > 0.3f)
			glgui::CBaseControl::PaintSheet(m_iCursors, mx-20, my-20, 80, 40, 80, 80, 80, 40, 256, 128);
		else
			glgui::CBaseControl::PaintSheet(m_iCursors, mx-20, my-20, 80, 40, 160, 40, 80, 40, 256, 128);
	}
	else if (m_eMouseCursor == MOUSECURSOR_AIMINVALID)
		glgui::CBaseControl::PaintSheet(m_iCursors, mx-20, my-20, 80, 40, 160, 80, 80, 40, 256, 128);

	glEnable(GL_DEPTH_TEST);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();   

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glPopAttrib();
}

void LoadLevel(class CCommand* pCommand, eastl::vector<eastl::string16>& asTokens, const eastl::string16& sCommand)
{
	if (asTokens.size() == 1)
	{
		if (!GameServer())
		{
			TMsg("Use load_level 'levelpath' to specify the level.\n");
			return;
		}

		CDigitanksLevel* pLevel = CDigitanksGame::GetLevel(CVar::GetCVarValue(L"game_level"));

		if (!pLevel)
		{
			TMsg(eastl::string16(L"Can't find file '") + CVar::GetCVarValue(L"game_level") + L"'.\n");
			return;
		}

		DigitanksWindow()->CreateGame(pLevel->GetGameType());
		return;
	}

	CDigitanksLevel* pLevel = CDigitanksGame::GetLevel(asTokens[1]);

	if (!pLevel)
	{
		TMsg(eastl::string16(L"Can't find file '") + asTokens[1] + L"'.\n");
		return;
	}

	CVar::SetCVar(L"game_level", pLevel->GetFile());

	DigitanksWindow()->CreateGame(pLevel->GetGameType());

	CApplication::CloseConsole();
}

CCommand load_level("load_level", ::LoadLevel);
CVar game_type("game_type", "");

void CDigitanksWindow::CreateGame(gametype_t eRequestedGameType)
{
	gametype_t eGameType = GAMETYPE_MENU;
	if (eRequestedGameType == GAMETYPE_FROM_CVAR)
	{
		if (game_type.GetValue() == L"menu")
			eGameType = GAMETYPE_MENU;
		else if (game_type.GetValue() == L"artillery")
			eGameType = GAMETYPE_ARTILLERY;
		else if (game_type.GetValue() == L"strategy")
			eGameType = GAMETYPE_STANDARD;
		else if (game_type.GetValue() == L"campaign")
			eGameType = GAMETYPE_CAMPAIGN;
		else
			eGameType = GAMETYPE_EMPTY;
	}
	else if (eRequestedGameType == GAMETYPE_FROM_LOBBY)
		eGameType = (gametype_t)_wtoi(CGameLobbyClient::L_GetInfoValue(L"gametype").c_str());
	else
		eGameType = eRequestedGameType;

	if (eGameType == GAMETYPE_MENU)
		game_type.SetValue(L"menu");
	else if (eGameType == GAMETYPE_ARTILLERY)
		game_type.SetValue(L"artillery");
	else if (eGameType == GAMETYPE_STANDARD)
		game_type.SetValue(L"strategy");
	else if (eGameType == GAMETYPE_CAMPAIGN)
		game_type.SetValue(L"campaign");
	else
		game_type.SetValue(L"empty");

	// Suppress all network commands until the game is done loading.
	GameNetwork()->SendCommands(false);

	RenderLoading();

	if (eGameType != GAMETYPE_MENU)
		CSoundLibrary::StopMusic();

	if (eGameType == GAMETYPE_MENU)
	{
		if (!CSoundLibrary::IsMusicPlaying() && !HasCommandLineSwitch("--no-music"))
			CSoundLibrary::PlayMusic(L"sound/assemble-for-victory.ogg", true);
	}
	else if (!HasCommandLineSwitch("--no-music"))
		CSoundLibrary::PlayMusic(L"sound/network-rise-network-fall.ogg", true);

	mtsrand((size_t)time(NULL));

	const char* pszPort = GetCommandLineSwitchValue("--port");
	int iPort = pszPort?atoi(pszPort):0;

	if (!m_pGameServer)
	{
		m_pHUD = new CHUD();
		glgui::CRootPanel::Get()->AddControl(m_pHUD);

		m_pGameServer = new CGameServer(this);

		if (!m_pInstructor)
			m_pInstructor = new CInstructor();
	}

	if (GameServer())
	{
		GameServer()->SetServerType(m_eServerType);
		if (eGameType == GAMETYPE_MENU)
			GameServer()->SetServerType(SERVER_LOCAL);
		GameServer()->SetServerPort(iPort);
		GameServer()->Initialize();

		GameNetwork()->SetCallbacks(m_pGameServer, CGameServer::ClientConnectCallback, CGameServer::ClientDisconnectCallback);
	}

	if (GameNetwork()->IsHost() && DigitanksGame())
	{
		GameServer()->SetupFromLobby(eRequestedGameType == GAMETYPE_FROM_LOBBY);
		DigitanksGame()->SetupGame(eGameType);
	}

	// Now turn the network on and connect all clients.
	GameNetwork()->SendCommands(true);

	if (GameNetwork()->IsHost())
		GameServer()->ClientConnect(NETWORK_TOCLIENTS);

	// Must set player nickname after teams have been set up or it won't stick.
	if (GameServer())
		GameServer()->SetPlayerNickname(GetPlayerNickname());

	glgui::CRootPanel::Get()->Layout();

	m_pMainMenu->SetVisible(eGameType == GAMETYPE_MENU);
	m_pVictory->SetVisible(false);
}

void CDigitanksWindow::DestroyGame()
{
	TMsg(L"Destroying game.\n");

	RenderLoading();

	if (m_pGameServer)
		delete m_pGameServer;

	if (m_pHUD)
	{
		glgui::CRootPanel::Get()->RemoveControl(m_pHUD);
		delete m_pHUD;
	}

	if (m_pInstructor)
		delete m_pInstructor;

	m_pGameServer = NULL;
	m_pHUD = NULL;
	m_pInstructor = NULL;

	CSoundLibrary::StopMusic();
}

void CDigitanksWindow::NewCampaign()
{
	if (!m_pCampaign)
		m_pCampaign = new CCampaignData(CCampaignInfo::GetCampaignInfo());

	CVar::SetCVar("game_level", m_pCampaign->BeginCampaign());

	DigitanksWindow()->SetServerType(SERVER_LOCAL);
	DigitanksWindow()->CreateGame(GAMETYPE_CAMPAIGN);
}

void CDigitanksWindow::RestartCampaignLevel()
{
	Restart(GAMETYPE_CAMPAIGN);
}

void CDigitanksWindow::NextCampaignLevel()
{
	eastl::string sNextLevel = m_pCampaign->ProceedToNextLevel();
	if (sNextLevel.length() == 0)
		Restart(GAMETYPE_MENU);
	else
	{
		GetVictoryPanel()->SetVisible(false);
		CVar::SetCVar("game_level", sNextLevel);
		Restart(GAMETYPE_CAMPAIGN);
	}

	m_pCampaign->SaveData(GetAppDataDirectory(L"Digitanks", L"campaign.txt"));
}

void CDigitanksWindow::ContinueCampaign()
{
	eastl::string sLevel = m_pCampaign->GetCurrentLevelFile();
	if (sLevel.length() != 0)
	{
		GetVictoryPanel()->SetVisible(false);
		CVar::SetCVar("game_level", sLevel);
		Restart(GAMETYPE_CAMPAIGN);
	}

	m_pCampaign->SaveData(GetAppDataDirectory(L"Digitanks", L"campaign.txt"));
}

void CDigitanksWindow::Restart(gametype_t eRestartAction)
{
	m_eRestartAction = eRestartAction;
	GameServer()->Halt();
}

void CDigitanksWindow::Run()
{
	CreateGame(GAMETYPE_MENU);

#ifndef DT_COMPETITION
	if (!IsRegistered())
	{
		m_pMainMenu->SetVisible(false);
		m_pPurchase->OpeningApplication();
	}
#endif

	while (IsOpen())
	{
		CProfiler::BeginFrame();

		if (true)
		{
			TPROF("CDigitanksWindow::Run");

			SetMouseCursor(MOUSECURSOR_NONE);

			ConstrainMouse();

			if (GameServer()->IsHalting())
			{
				DestroyGame();
				CreateGame(m_eRestartAction);

				m_eRestartAction = GAMETYPE_MENU;
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
					DestroyGame();
					CreateGame(GAMETYPE_MENU);
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

void CDigitanksWindow::ConstrainMouse()
{
#ifdef _WIN32
	if (IsFullscreen())
		return;

	HWND hWindow = FindWindow(NULL, L"Digitanks!");

	if (!hWindow)
		return;

	HWND hActiveWindow = GetActiveWindow();
	if (ShouldConstrainMouse())
	{
		RECT rc;
		GetClientRect(hWindow, &rc);

		// Convert the client area to screen coordinates.
		POINT pt = { rc.left, rc.top };
		POINT pt2 = { rc.right, rc.bottom };
		ClientToScreen(hWindow, &pt);
		ClientToScreen(hWindow, &pt2);
		SetRect(&rc, pt.x, pt.y, pt2.x, pt2.y);

		// Confine the cursor.
		ClipCursor(&rc);
	}
	else
		ClipCursor(NULL);
#endif
}

void CDigitanksWindow::Render()
{
	if (!GameServer())
		return;

	TPROF("CDigitanksWindow::Render");

	GameServer()->Render();

	if (true)
	{
		TPROF("GUI");
		glgui::CRootPanel::Get()->Think(GameServer()->GetGameTime());
		glgui::CRootPanel::Get()->Paint(0, 0, (int)m_iWindowWidth, (int)m_iWindowHeight);
	}

	RenderMouseCursor();
}

int CDigitanksWindow::WindowClose()
{
	if (m_pPurchase->IsVisible())
		return GL_TRUE;

	CloseApplication();
	return GL_FALSE;
}

void CDigitanksWindow::WindowResize(int w, int h)
{
	if (GameServer() && GameServer()->GetRenderer())
		GameServer()->GetRenderer()->SetSize(w, h);

	BaseClass::WindowResize(w, h);
}

bool CDigitanksWindow::ShouldConstrainMouse()
{
	if (!m_bConstrainMouse)
		return false;

#ifdef _WIN32
	HWND hWindow = FindWindow(NULL, L"Digitanks!");
	HWND hActiveWindow = GetActiveWindow();

	if (hActiveWindow != hWindow)
		return false;
#endif

	if (GameServer() && GameServer()->IsLoading())
		return false;
	
	if (!DigitanksGame() || DigitanksGame()->GetGameType() == GAMETYPE_MENU)
		return false;
	
	if (GetMenu()->IsVisible())
		return false;
	
	if (IsConsoleOpen())
		return false;

	return true;
}

bool CDigitanksWindow::GetMouseGridPosition(Vector& vecPoint, CBaseEntity** pHit, int iCollisionGroup)
{
	if (!DigitanksGame()->GetTerrain())
		return false;

	int x, y;
	glgui::CRootPanel::Get()->GetFullscreenMousePos(x, y);

	Vector vecWorld = GameServer()->GetRenderer()->WorldPosition(Vector((float)x, (float)y, 1));

	Vector vecCameraVector = GameServer()->GetCamera()->GetCameraPosition();

	Vector vecRay = (vecWorld - vecCameraVector).Normalized();

	return GameServer()->GetGame()->TraceLine(vecCameraVector, vecCameraVector+vecRay*1000, vecPoint, pHit, iCollisionGroup);
}

void CDigitanksWindow::GameOver(bool bPlayerWon)
{
	DigitanksGame()->SetControlMode(MODE_NONE);
	GetInstructor()->SetActive(false);
	m_pVictory->GameOver(bPlayerWon);
}

void CDigitanksWindow::OnClientDisconnect(int iClient)
{
	if (iClient == GameNetwork()->GetClientID())
		Restart(GAMETYPE_MENU);
}

void CDigitanksWindow::CloseApplication()
{
#ifdef DT_COMPETITION
	exit(0);
#endif

	if (IsRegistered())
		exit(0);

	if (m_pPurchase->IsVisible())
		exit(0);

	m_pMenu->SetVisible(false);
	m_pMainMenu->SetVisible(false);
	m_pPurchase->ClosingApplication();

	SaveConfig();
}

void CDigitanksWindow::SaveConfig()
{
	c.add<float>(L"soundvolume", GetSoundVolume());
	c.add<float>(L"musicvolume", GetMusicVolume());
	c.add<bool>(L"windowed", !m_bCfgFullscreen);
	c.add<bool>(L"constrainmouse", m_bConstrainMouse);
	c.add<bool>(L"contextualcommands", m_bContextualCommands);
	c.add<bool>(L"reversespacebar", m_bReverseSpacebar);
	c.add<bool>(L"useframebuffers", m_bWantsFramebuffers);
	c.add<bool>(L"useshaders", m_bWantsShaders);
	c.add<int>(L"width", m_iCfgWidth);
	c.add<int>(L"height", m_iCfgHeight);
	c.add<int>(L"installid", m_iInstallID);
	c.add<eastl::string16>(L"nickname", m_sNickname);
	std::wofstream o;
	o.open(GetAppDataDirectory(L"Digitanks", L"options.cfg").c_str(), std::ios_base::out);
	o << c;

	TMsg(L"Saved config.\n");
}

CInstructor* CDigitanksWindow::GetInstructor()
{
	if (!m_pInstructor)
		m_pInstructor = new CInstructor();

	return m_pInstructor;
}

void CDigitanksWindow::SetSoundVolume(float flSoundVolume)
{
	m_flSoundVolume = flSoundVolume;
	CSoundLibrary::SetSoundVolume(m_flSoundVolume);
}

void CDigitanksWindow::SetMusicVolume(float flMusicVolume)
{
	m_flMusicVolume = flMusicVolume;
	CSoundLibrary::SetMusicVolume(m_flMusicVolume);
}

void CDigitanksWindow::BeginProgress()
{
}

void CDigitanksWindow::SetAction(const wchar_t* pszAction, size_t iTotalProgress)
{
	m_sAction = pszAction;
	m_iTotalProgress = iTotalProgress;

	WorkProgress(0, true);
}

void CDigitanksWindow::WorkProgress(size_t iProgress, bool bForceDraw)
{
	if (!GameServer()->IsLoading())
		return;

	m_iProgress = iProgress;

	static float flLastTime = 0;

	CNetwork::Think();

	// Don't update too often or it'll slow us down just because of the updates.
	if (!bForceDraw && GetTime() - flLastTime < 0.5f)
		return;

	RenderLoading();

	flLastTime = GetTime();
}

void CDigitanksWindow::EndProgress()
{
}
