#pragma once

#include <tengine/game/entities/baseentity.h>

class CCommandButton
{
public:
	void                     SetLabel(const tstring& sLabel) { m_sLabel = sLabel; }
	const tstring &          GetLabel() { return m_sLabel; }

	void                     SetCommand(const tstring& sCommand) { m_sCommand = sCommand; }
	const tstring &          GetCommand() { return m_sCommand; }

private:
	tstring                  m_sLabel;
	tstring                  m_sCommand;
};

#define COMMAND_BUTTONS 6

class CCommandMenu
{
public:
	CCommandMenu(CBaseEntity* pOwner, class CPlayerCharacter* pRequester);
	~CCommandMenu();

public:
	void                     SetButton(size_t i, const tstring& sLabel, const tstring& sCommand);

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

	CCommandButton*          m_apButtons[COMMAND_BUTTONS];

	bool                     m_bMouseInMenu;
	size_t                   m_iMouseInButton;
};
