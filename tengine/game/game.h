#ifndef GAME_H
#define GAME_H

#include <EASTL/vector.h>

#include <network/replication.h>

#include "baseentity.h"
#include "gameserver.h"
#include "entities/player.h"

class CGame : public CBaseEntity, public INetworkListener
{
	REGISTER_ENTITY_CLASS(CGame, CBaseEntity);

public:
												CGame();
	virtual										~CGame();

public:
	virtual void								Spawn();

	virtual void								Simulate() {};
	virtual bool								ShouldRunSimulation() { return true; };

	virtual void								RegisterNetworkFunctions();

	virtual void								OnClientConnect(int iClient);
	virtual void								OnClientEnterGame(int iClient);
	virtual void								OnClientDisconnect(int iClient);

	virtual void								EnterGame();

	void										AddPlayer(CPlayer* pPlayer);
	void										RemovePlayer(CPlayer* pPlayer);

	virtual void								OnDeleted();

	virtual void								OnTakeDamage(class CBaseEntity* pVictim, class CBaseEntity* pAttacker, class CBaseEntity* pInflictor, float flDamage, bool bDirectHit, bool bKilled) {};
	virtual void								OnDeleted(class CBaseEntity* pEntity);

	virtual bool								TraceLine(const Vector& s1, const Vector& s2, Vector& vecHit, CBaseEntity** pHit, int iCollisionGroup = 0);

	size_t										GetNumPlayers() const { return m_ahPlayers.size(); };
	CPlayer*									GetPlayer(size_t i) const;
	bool										IsTeamControlledByMe(const class CTeam* pTeam);

	const eastl::vector<CEntityHandle<CPlayer> >&	GetLocalPlayers();
	size_t										GetNumLocalPlayers();
	CPlayer*									GetLocalPlayer(size_t i);
	CPlayer*									GetLocalPlayer();

	static void									ClearLocalPlayers(CNetworkedVariableBase* pVariable);

	virtual bool								AllowCheats();

protected:
	CNetworkedSTLVector<CEntityHandle<CPlayer> >	m_ahPlayers;

	eastl::vector<CEntityHandle<CPlayer> >		m_ahLocalPlayers;
};

inline class CGame* Game()
{
	CGameServer* pGameServer = GameServer();
	if (!pGameServer)
		return NULL;

	CGame* pGame = pGameServer->GetGame();
	if (!pGame)
		return NULL;

	return pGame;
}

#endif
