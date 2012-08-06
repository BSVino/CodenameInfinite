#ifndef TINKER_PLAYER_H
#define TINKER_PLAYER_H

#include <tengine/game/entities/baseentity.h>

class CCharacter;

class CPlayer : public CBaseEntity
{
	REGISTER_ENTITY_CLASS(CPlayer, CBaseEntity);

public:
									CPlayer();

public:
	virtual void					MouseMotion(int x, int y);
	virtual void					MouseInput(int iButton, int iState) {};
	virtual void					KeyPress(int c);
	virtual void					KeyRelease(int c);
	virtual void					JoystickButtonPress(int iJoystick, int c);
	virtual void					JoystickButtonRelease(int iJoystick, int c) {};
	virtual void					JoystickAxis(int iJoystick, int iAxis, float flValue, float flChange);

	void							SetCharacter(CCharacter* pCharacter);
	CCharacter*						GetCharacter() const;

	void							SetClient(int iClient);
	int								GetClient() const { return m_iClient; };
	void							SetInstallID(int i) { m_iInstallID = i; };
	int								GetInstallID() const { return m_iInstallID; };

	void							SetPlayerName(const tstring& sName) { m_sPlayerName = sName; }
	const tstring&					GetPlayerName() const { return m_sPlayerName; }

protected:
	CNetworkedHandle<CCharacter>	m_hCharacter;

	CNetworkedVariable<int>			m_iClient;
	size_t							m_iInstallID;

	CNetworkedString				m_sPlayerName;
};

#endif
