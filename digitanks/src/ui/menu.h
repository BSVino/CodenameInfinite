#ifndef DT_MENU_H
#define DT_MENU_H

#include <common.h>
#include "glgui/glgui.h"

class CDigitanksMenu : public glgui::CPanel, public glgui::IEventListener
{
	DECLARE_CLASS(CDigitanksMenu, glgui::CPanel);

public:
									CDigitanksMenu();

public:
	virtual void					Layout();
	virtual void					Paint(int x, int y, int w, int h);

	virtual void					SetVisible(bool bVisible);

	EVENT_CALLBACK(CDigitanksMenu, StartTutorialBasics);
	EVENT_CALLBACK(CDigitanksMenu, StartTutorialBases);
	EVENT_CALLBACK(CDigitanksMenu, StartGame);
	EVENT_CALLBACK(CDigitanksMenu, Exit);

protected:
	glgui::CLabel*					m_pDigitanks;

	glgui::CScrollSelector<int>*	m_pDifficulty;
	glgui::CLabel*					m_pDifficultyLabel;

	glgui::CButton*					m_pStartTutorialBasics;
	glgui::CButton*					m_pStartTutorialBases;
	glgui::CButton*					m_pStartGame;
	glgui::CButton*					m_pExit;

	size_t							m_iLunarWorkshop;
};

class CVictoryPanel : public glgui::CPanel
{
	DECLARE_CLASS(CVictoryPanel, glgui::CPanel);

public:
									CVictoryPanel();

public:
	virtual void					Layout();
	virtual void					Paint(int x, int y, int w, int h);

	virtual bool					IsCursorListener() {return true;};
	virtual bool					MousePressed(int code, int mx, int my);
	virtual bool					KeyPressed(int iKey);

	virtual void					GameOver(bool bPlayerWon);

protected:
	glgui::CLabel*					m_pVictory;
};

class CDonatePanel : public glgui::CPanel, public glgui::IEventListener
{
	DECLARE_CLASS(CDonatePanel, glgui::CPanel);

public:
									CDonatePanel();

public:
	virtual void					Layout();
	virtual void					Paint(int x, int y, int w, int h);

	virtual void					ClosingApplication();

	EVENT_CALLBACK(CDonatePanel, Donate);
	EVENT_CALLBACK(CDonatePanel, Exit);

protected:
	glgui::CLabel*					m_pDonate;

	glgui::CButton*					m_pDonateButton;
	glgui::CButton*					m_pExitButton;
};

class CStoryPanel : public glgui::CPanel
{
	DECLARE_CLASS(CStoryPanel, glgui::CPanel);

public:
									CStoryPanel();

public:
	virtual void					Layout();
	virtual void					Paint(int x, int y, int w, int h);

	virtual bool					IsCursorListener() {return true;};
	virtual bool					MousePressed(int code, int mx, int my);
	virtual bool					KeyPressed(int iKey);

protected:
	glgui::CLabel*					m_pStory;
};

#endif