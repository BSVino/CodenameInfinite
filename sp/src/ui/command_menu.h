#pragma once

#include <tengine/game/entities/baseentity.h>

class CCommandButton
{
public:
	CCommandButton();

public:
	void                     SetLabel(const tstring& sLabel) { m_sLabel = sLabel; }
	const tstring &          GetLabel() { return m_sLabel; }

	void                     SetCommand(const tstring& sCommand) { m_sCommand = sCommand; }
	const tstring &          GetCommand() { return m_sCommand; }

	void                     SetToolTip(const tstring& sToolTip) { m_sToolTip = sToolTip; }
	const tstring &          GetToolTip() { return m_sToolTip; }

	void                     SetEnabled(bool bEnabled) { m_bEnabled = bEnabled; }
	bool                     IsEnabled() { return m_bEnabled; }

private:
	tstring                  m_sLabel;
	tstring                  m_sCommand;
	tstring                  m_sToolTip;
	bool                     m_bEnabled;
};

#define COMMAND_BUTTONS 6

class CCommandMenu
{
public:
	CCommandMenu(CBaseEntity* pOwner, class CPlayerCharacter* pRequester);
	~CCommandMenu();

public:
	void                     SetTitle(const tstring& sTitle) { m_sTitle = sTitle; }
	void                     SetSubtitle(const tstring& sSubtitle) { m_sSubtitle = sSubtitle; }
	void                     SetButton(size_t i, const tstring& sLabel, const tstring& sCommand);
	void                     SetButtonEnabled(size_t i, bool bEnabled);
	void                     SetButtonToolTip(size_t i, const tstring& sToolTip);
	void                     SetProgressBar(double flCurrent, double flMax);
	void                     DisableProgressBar();

	void                     Think();

	void                     Render() const;

	bool                     MouseInput(int iButton, int iState);

	CBaseEntity*             GetOwner() const { return m_hOwner; }
	class CPlayerCharacter*  GetPlayerCharacter() const { return m_hPlayerCharacter; }

	bool                     WantsToClose() const;
	size_t                   GetNumActiveButtons() const;

	void                     GetVectors(TVector& vecLocalCenter, Vector& vecProjectionDirection, Vector& vecUp, Vector& vecRight, float& flProjectionDistance, float& flProjectionRadius) const;
	Vector2D                 GetButtonCenter(size_t i) const;
	float                    ButtonSize() const { return 0.3f; }

private:
	CEntityHandle<class CBaseEntity>           m_hOwner;
	CEntityHandle<class CPlayerCharacter>      m_hPlayerCharacter;

	tstring                  m_sTitle;
	tstring                  m_sSubtitle;

	CCommandButton*          m_apButtons[COMMAND_BUTTONS];

	bool                     m_bMouseInMenu;
	size_t                   m_iMouseInButton;

	double                   m_flCurrentProgress;
	double                   m_flMaxProgress;
};

