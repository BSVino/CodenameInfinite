#ifndef TINKER_PLAYER_H
#define TINKER_PLAYER_H

#include <tengine/game/baseentity.h>

class CCharacter;

class CPlayer : public CBaseEntity
{
	REGISTER_ENTITY_CLASS(CPlayer, CBaseEntity);

public:
									CPlayer();

public:
	virtual void					MouseMotion(int x, int y);
	virtual void					KeyPress(int c);
	virtual void					KeyRelease(int c);

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
