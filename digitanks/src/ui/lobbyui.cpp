#include "lobbyui.h"

#include <tinker/cvar.h>
#include <tinker/lobby/lobby_server.h>
#include <tinker/lobby/lobby_client.h>

#include <renderer/renderer.h>

#include "menu.h"
#include "digitankswindow.h"
#include "chatbox.h"

CVar lobby_gametype("lobby_gametype", "");

CLobbyPanel::CLobbyPanel()
	: CPanel(0, 0, 100, 100)
{
	SetVisible(false);

	m_pLobbyName = new glgui::CLabel(0, 0, 100, 100, L"Lobby");
	m_pLobbyName->SetFont(L"header", 30);
	AddControl(m_pLobbyName);

	m_pPlayerList = new glgui::CLabel(0, 0, 100, 100, L"Player List");
	m_pPlayerList->SetFont(L"header", 18);
	AddControl(m_pPlayerList);

	m_pDockPanel = new CDockPanel();
	m_pDockPanel->SetBGColor(Color(12, 13, 12, 0));
	AddControl(m_pDockPanel);

	m_pChatBox = new CChatBox(false);
	AddControl(m_pChatBox);

	m_pLeave = new glgui::CButton(0, 0, 100, 100, L"Leave Lobby");
	m_pLeave->SetClickedListener(this, LeaveLobby);
	m_pLeave->SetFont(L"header");
	AddControl(m_pLeave);

	m_pReady = new glgui::CButton(0, 0, 100, 100, L"Ready");
	m_pReady->SetClickedListener(this, PlayerReady);
	m_pReady->SetFont(L"header");
	AddControl(m_pReady);
}

void CLobbyPanel::Layout()
{
	size_t iWidth = DigitanksWindow()->GetWindowWidth();
	size_t iHeight = DigitanksWindow()->GetWindowHeight();

	SetSize(924, 668);
	SetPos(iWidth/2-GetWidth()/2, iHeight/2-GetHeight()/2);

	// Find the lobby leader's name
	for (size_t i = 0; i < CGameLobbyClient::GetNumPlayers(); i++)
	{
		CLobbyPlayer* pPlayer = CGameLobbyClient::GetPlayer(i);
		if (pPlayer->GetInfoValue(L"host") == L"1")
		{
			m_pLobbyName->SetText(pPlayer->GetInfoValue(L"name") + L"'s Lobby");
			break;
		}
	}

	m_pLobbyName->SetSize(GetWidth(), 30);
	m_pLobbyName->SetPos(0, 0);
	m_pLobbyName->SetAlign(glgui::CLabel::TA_MIDDLECENTER);

	m_pPlayerList->SetSize(260, 20);
	m_pPlayerList->SetPos(925 - 280, 30);
	m_pPlayerList->SetAlign(glgui::CLabel::TA_MIDDLECENTER);

	m_pDockPanel->SetSize(375, 480);
	m_pDockPanel->SetPos(20, 50);

	m_pChatBox->SetSize(565, 120); 
	m_pChatBox->SetPos(20, 520);

	m_pLeave->SetSize(100, 35);
	m_pLeave->SetPos(925 - 320, 668 - 55);

	m_pReady->SetSize(180, 35);
	m_pReady->SetPos(925 - 200, 668 - 55);
	if (CGameLobbyClient::GetPlayerByClient(CNetwork::GetClientID()))
	{
		bool bReady = !!_wtoi(CGameLobbyClient::GetPlayerByClient(CNetwork::GetClientID())->GetInfoValue(L"ready").c_str());
		if (bReady)
			m_pReady->SetText(L"Not Ready");
		else
			m_pReady->SetText(L"Ready");
	}
	else
		m_pReady->SetText(L"Ready");

	for (size_t i = 0; i < m_apPlayerPanels.size(); i++)
	{
		m_apPlayerPanels[i]->Destructor();
		m_apPlayerPanels[i]->Delete();
	}

	m_apPlayerPanels.clear();

	for (size_t i = 0; i < CGameLobbyClient::GetNumPlayers(); i++)
	{
		m_apPlayerPanels.push_back(new CPlayerPanel());
		CPlayerPanel* pPanel = m_apPlayerPanels[m_apPlayerPanels.size()-1];
		AddControl(pPanel);
		pPanel->SetPlayer(CGameLobbyClient::GetPlayer(i)->iClient);
	}

	BaseClass::Layout();
}

void CLobbyPanel::Paint(int x, int y, int w, int h)
{
	glgui::CRootPanel::PaintRect(x, y, w, h, Color(12, 13, 12, 255));

	BaseClass::Paint(x, y, w, h);
}

void CLobbyPanel::CreateLobby()
{
	CGameLobbyClient::SetLobbyUpdateCallback(this, &LobbyUpdateCallback);
	CGameLobbyClient::SetBeginGameCallback(&BeginGameCallback);

	const char* pszPort = DigitanksWindow()->GetCommandLineSwitchValue("--port");
	int iPort = pszPort?atoi(pszPort):0;

	m_iLobby = CGameLobbyServer::CreateLobby(iPort);

	CGameLobbyClient::JoinLobby(m_iLobby);
	CGameLobbyClient::UpdatePlayerInfo(L"host", L"1");
	CGameLobbyClient::UpdateLobbyInfo(L"gametype", sprintf(L"%d", (gametype_t)lobby_gametype.GetInt()));
	UpdatePlayerInfo();

	if ((gametype_t)lobby_gametype.GetInt() == GAMETYPE_ARTILLERY)
		m_pDockPanel->SetDockedPanel(new CArtilleryGamePanel(true));
	else
		m_pDockPanel->SetDockedPanel(new CStrategyGamePanel(true));

	SetVisible(true);
	DigitanksWindow()->GetMainMenu()->SetVisible(false);
}

void CLobbyPanel::ConnectToLocalLobby(const eastl::string16& sHost)
{
	CGameLobbyClient::SetLobbyUpdateCallback(this, &LobbyUpdateCallback);
	CGameLobbyClient::SetBeginGameCallback(&BeginGameCallback);

	const char* pszPort = DigitanksWindow()->GetCommandLineSwitchValue("--port");
	int iPort = pszPort?atoi(pszPort):0;

	CNetwork::ConnectToHost(convertstring<char16_t, char>(sHost).c_str(), iPort);
	if (!CNetwork::IsConnected())
		return;

	m_pDockPanel->SetDockedPanel(new CInfoPanel());

	SetVisible(true);
	DigitanksWindow()->GetMainMenu()->SetVisible(false);

	UpdatePlayerInfo();
}

void CLobbyPanel::UpdatePlayerInfo()
{
	CGameLobbyClient::UpdatePlayerInfo(L"name", DigitanksWindow()->GetPlayerNickname());
	CGameLobbyClient::UpdatePlayerInfo(L"ready", L"0");
}

void CLobbyPanel::LeaveLobbyCallback()
{
	if (CNetwork::IsHost())
		CGameLobbyServer::DestroyLobby(m_iLobby);
	else
		CGameLobbyClient::LeaveLobby();

	SetVisible(false);
	DigitanksWindow()->GetMainMenu()->SetVisible(true);
}

void CLobbyPanel::LobbyUpdateCallback(INetworkListener*, class CNetworkParameters*)
{
	DigitanksWindow()->GetLobbyPanel()->LobbyUpdate();
}

void CLobbyPanel::LobbyUpdate()
{
	Layout();
}

void CLobbyPanel::PlayerReadyCallback()
{
	bool bReady = !!_wtoi(CGameLobbyClient::GetPlayerByClient(CNetwork::GetClientID())->GetInfoValue(L"ready").c_str());

	if (bReady)
		CGameLobbyClient::UpdatePlayerInfo(L"ready", L"0");
	else
		CGameLobbyClient::UpdatePlayerInfo(L"ready", L"1");
}

void CLobbyPanel::BeginGameCallback(INetworkListener*, class CNetworkParameters*)
{
	DigitanksWindow()->DestroyGame();

	CVar::SetCVar(L"game_level", CGameLobbyClient::GetInfoValue(L"level_file"));

	DigitanksWindow()->CreateGame(GAMETYPE_FROM_LOBBY);

	DigitanksWindow()->GetLobbyPanel()->SetVisible(false);
}

CInfoPanel::CInfoPanel()
	: CPanel(0, 0, 570, 520)
{
	m_pLobbyDescription = new glgui::CLabel(0, 0, 32, 32, L"");
	m_pLobbyDescription->SetWrap(true);
	m_pLobbyDescription->SetFont(L"text");
	m_pLobbyDescription->SetAlign(glgui::CLabel::TA_TOPLEFT);
	AddControl(m_pLobbyDescription);
}

void CInfoPanel::Layout()
{
	m_pLobbyDescription->SetSize(GetWidth()-40, 80);
	m_pLobbyDescription->SetPos(20, 20);

	gametype_t eGameType = (gametype_t)_wtoi(CGameLobbyClient::GetInfoValue(L"gametype").c_str());

	if (eGameType == GAMETYPE_ARTILLERY)
		m_pLobbyDescription->SetText(L"Game Mode: Artillery\n");
	else
		m_pLobbyDescription->SetText(L"Game Mode: Standard\n");

	m_pLobbyDescription->AppendText(eastl::string16(L"Level: ") + CGameLobbyClient::GetInfoValue(L"level") + L"\n");

	if (eGameType == GAMETYPE_ARTILLERY)
	{
		m_pLobbyDescription->AppendText(eastl::string16(L"Tanks per player: ") + CGameLobbyClient::GetInfoValue(L"tanks") + L"\n");

		eastl::string16 sHeight = CGameLobbyClient::GetInfoValue(L"terrain");
		float flHeight = (float)_wtof(sHeight.c_str());
		if (fabs(flHeight - 10.0f) < 0.5f)
			sHeight = L"Flatty";
		else if (fabs(flHeight - 50.0f) < 0.5f)
			sHeight = L"Hilly";
		else if (fabs(flHeight - 80.0f) < 0.5f)
			sHeight = L"Mountainy";
		else if (fabs(flHeight - 120.0f) < 0.5f)
			sHeight = L"Everesty";
		m_pLobbyDescription->AppendText(eastl::string16(L"Terrain height: ") + sHeight + L"\n");
	}

	BaseClass::Layout();
}

CPlayerPanel::CPlayerPanel()
	: CPanel(0, 0, 100, 100)
{
	m_pName = new glgui::CLabel(0, 0, 100, 100, L"Player");
	m_pName->SetFont(L"text");
	AddControl(m_pName);
}

void CPlayerPanel::Layout()
{
	SetSize(260, 60);
	SetPos(925 - 280, 70 + 80*m_iLobbyPlayer);

	m_pName->SetSize(100, 60);
	m_pName->SetPos(50, 0);
	m_pName->SetAlign(glgui::CLabel::TA_LEFTCENTER);
}

void CPlayerPanel::Paint(int x, int y, int w, int h)
{
	glgui::CRootPanel::PaintRect(x, y, w, h, glgui::g_clrBox);

	BaseClass::Paint(x, y, w, h);
}

void CPlayerPanel::SetPlayer(size_t iClient)
{
	CLobbyPlayer* pPlayer = CGameLobbyClient::GetPlayerByClient(iClient);

	assert(pPlayer);
	if (!pPlayer)
		return;

	m_iLobbyPlayer = CGameLobbyClient::GetPlayerIndex(iClient);

	eastl::string16 sName = pPlayer->GetInfoValue(L"name");
	if (sName.length() == 0)
		sName = L"Player";

	eastl::string16 sHost = pPlayer->GetInfoValue(L"host");
	if (sHost == L"1")
		sName += L" - Host";

	m_pName->SetText(sName);

	Layout();
}