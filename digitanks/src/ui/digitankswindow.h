#ifndef DT_DIGITANKSWINDOW_H
#define DT_DIGITANKSWINDOW_H

#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <vector.h>
#include <color.h>

#include <game/digitanks/digitanksgame.h>
#include <tinker/application.h>

class CDigitanksWindow : public CApplication
{
	DECLARE_CLASS(CDigitanksWindow, CApplication);

public:
								CDigitanksWindow(int argc, char** argv);
	virtual 					~CDigitanksWindow();

public:
	void						OpenWindow();

	virtual eastl::string		WindowTitle() { return "Digitanks!"; }

	void						InitUI();

	void						SetPlayers(int iPlayers) { m_iPlayers = iPlayers; };
	void						SetTanks(int iTanks) { m_iTanks = iTanks; };
	void						SetServerType(servertype_t eServerType) { m_eServerType = eServerType; };
	void						SetConnectHost(const eastl::string16 sHost) { m_sConnectHost = sHost; };

	void						RenderLoading();

	void						CreateGame(gametype_t eGameType);
	void						DestroyGame();

	void						Run();	// Doesn't return

	void						ConstrainMouse();

	void						Layout();

	virtual void				Render();
	virtual void				WindowResize(int x, int y);
	virtual void				MouseMotion(int x, int y);
	virtual void				MouseInput(int iButton, int iState);
	virtual void				MouseWheel(int iState);

	virtual void				KeyPress(int c);
	virtual void				KeyRelease(int c);
	virtual void				CharPress(int c);
	virtual void				CharRelease(int c);

	bool						GetBoxSelection(size_t& iX, size_t& iY, size_t& iX2, size_t& iY2);

	int							GetMouseCurrentX() { return m_iMouseCurrentX; };
	int							GetMouseCurrentY() { return m_iMouseCurrentX; };

	void						SetConfigWindowDimensions(int iWidth, int iHeight) { m_iCfgWidth = iWidth; m_iCfgHeight = iHeight; };
	void						SetConfigFullscreen(bool bFullscreen) { m_bCfgFullscreen = bFullscreen; };
	void						SetConstrainMouse(bool bConstrain) { m_bConstrainMouse = bConstrain; };
	bool						ShouldConstrainMouse() { return m_bConstrainMouse; };

	void						SetWantsFramebuffers(bool bWantsFramebuffers) { m_bWantsFramebuffers = bWantsFramebuffers; }
	bool						WantsFramebuffers() { return m_bWantsFramebuffers; }

	void						SetWantsShaders(bool bWantsShaders) { m_bWantsShaders = bWantsShaders; }
	bool						WantsShaders() { return m_bWantsShaders; }

	bool						GetMouseGridPosition(Vector& vecPoint, CBaseEntity** pHit = NULL, int iCollisionGroup = 0);

	void						GameOver(bool bPlayerWon);

	void						CloseApplication();

	void						SaveConfig();

	class CMainMenu*			GetMainMenu() { return m_pMainMenu; };
	class CDigitanksMenu*		GetMenu() { return m_pMenu; };
	class CGameServer*			GetGameServer() { return m_pGameServer; };
	class CHUD*					GetHUD() { return m_pHUD; };
	class CInstructor*			GetInstructor();
	class CVictoryPanel*		GetVictoryPanel() { return m_pVictory; };
	class CStoryPanel*			GetStoryPanel() { return m_pStory; };

	float						GetSoundVolume() { return m_flSoundVolume; };
	void						SetSoundVolume(float flSoundVolume);

	float						GetMusicVolume() { return m_flMusicVolume; };
	void						SetMusicVolume(float flMusicVolume);

protected:
	int							m_iMouseLastX;
	int							m_iMouseLastY;

	size_t						m_iLoading;

	class CMainMenu*			m_pMainMenu;
	class CDigitanksMenu*		m_pMenu;
	class CVictoryPanel*		m_pVictory;
	class CPurchasePanel*		m_pPurchase;
	class CStoryPanel*			m_pStory;

	int							m_iPlayers;
	int							m_iTanks;

	servertype_t				m_eServerType;
	eastl::string16				m_sConnectHost;

	class CGameServer*			m_pGameServer;

	class CHUD*					m_pHUD;

	class CInstructor*			m_pInstructor;

	bool						m_bBoxSelect;
	int							m_iMouseInitialX;
	int							m_iMouseInitialY;
	int							m_iMouseCurrentX;
	int							m_iMouseCurrentY;

	int							m_iMouseMoved;

	bool						m_bMouseDownInGUI;

	bool						m_bCheatsOn;

	bool						m_bCfgFullscreen;
	int							m_iCfgWidth;
	int							m_iCfgHeight;
	bool						m_bConstrainMouse;
	bool						m_bWantsFramebuffers;
	bool						m_bWantsShaders;

	float						m_flSoundVolume;
	float						m_flMusicVolume;
};

inline CDigitanksWindow* DigitanksWindow()
{
	return dynamic_cast<CDigitanksWindow*>(CDigitanksWindow::Get());
}

#endif