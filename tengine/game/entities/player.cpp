#include "player.h"

#include <game/entities/game.h>
#include <tinker/cvar.h>
#include <tinker/keys.h>

#include "character.h"

REGISTER_ENTITY(CPlayer);

NETVAR_TABLE_BEGIN(CPlayer);
	NETVAR_DEFINE(CEntityHandle, m_hCharacter);
	NETVAR_DEFINE_CALLBACK(int, m_iClient, &CGame::ClearLocalPlayers);
	NETVAR_DEFINE(tstring, m_sPlayerName);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CPlayer);
	SAVEDATA_DEFINE(CSaveData::DATA_NETVAR, CEntityHandle<CCharacter>, m_hCharacter);
	SAVEDATA_DEFINE(CSaveData::DATA_NETVAR, int, m_iClient);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, size_t, m_iInstallID);
	SAVEDATA_DEFINE(CSaveData::DATA_NETVAR, int, m_sPlayerName);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CPlayer);
INPUTS_TABLE_END();

CPlayer::CPlayer()
{
	m_iClient = NETWORK_LOCAL;
}

void NoClip(class CCommand* pCommand, tvector<tstring>& asTokens, const tstring& sCommand)
{
	if (!CVar::GetCVarBool("cheats"))
	{
		TMsg("Noclip is not allowed with cheats off.\n");
		return;
	}

	if (!Game())
		return;

	if (!Game()->GetLocalPlayer())
		return;

	if (!Game()->GetLocalPlayer()->GetCharacter())
		return;

	CCharacter* pCharacter = Game()->GetLocalPlayer()->GetCharacter();
	pCharacter->SetNoClip(!pCharacter->GetNoClip());

	if (pCharacter->GetNoClip())
		TMsg("NoClip ON\n");
	else
		TMsg("NoClip OFF\n");
}

CCommand noclip("noclip", ::NoClip);

CVar m_sensitivity("m_sensitivity", "5");

void CPlayer::MouseMotion(int x, int y)
{
	if (!m_hCharacter)
		return;

	EAngle angDirection = m_hCharacter->GetViewAngles();

	angDirection.y += (x/m_sensitivity.GetFloat());
	angDirection.p -= (y/m_sensitivity.GetFloat());

	if (angDirection.p > 89)
		angDirection.p = 89;

	if (angDirection.p < -89)
		angDirection.p = -89;

	while (angDirection.y > 180)
		angDirection.y -= 360;

	while (angDirection.y < -180)
		angDirection.y += 360;

	m_hCharacter->SetViewAngles(angDirection);
}

void CPlayer::KeyPress(int c)
{
	if (!m_hCharacter)
		return;

	if (c == 'W')
		m_hCharacter->Move(MOVE_FORWARD);
	if (c == 'S')
		m_hCharacter->Move(MOVE_BACKWARD);
	if (c == 'D')
		m_hCharacter->Move(MOVE_RIGHT);
	if (c == 'A')
		m_hCharacter->Move(MOVE_LEFT);

	if (c == ' ')
		m_hCharacter->Jump();
}

void CPlayer::KeyRelease(int c)
{
	if (!m_hCharacter)
		return;

	if (c == 'W')
		m_hCharacter->StopMove(MOVE_FORWARD);
	if (c == 'S')
		m_hCharacter->StopMove(MOVE_BACKWARD);
	if (c == 'D')
		m_hCharacter->StopMove(MOVE_RIGHT);
	if (c == 'A')
		m_hCharacter->StopMove(MOVE_LEFT);
}

void CPlayer::JoystickButtonPress(int iJoystick, int c)
{
	if (c == TINKER_KEY_JOYSTICK_2)
		m_hCharacter->Jump();
}

void CPlayer::JoystickAxis(int iJoystick, int iAxis, float flValue, float flChange)
{
	if (iAxis == 0)
	{
		if (flValue < -0.1f)
			m_hCharacter->Move(MOVE_LEFT);
		else if (flValue > 0.1f)
			m_hCharacter->Move(MOVE_RIGHT);
		else
		{
			m_hCharacter->StopMove(MOVE_LEFT);
			m_hCharacter->StopMove(MOVE_RIGHT);
		}
	}
	else if (iAxis == 1)
	{
		if (flValue < -0.1f)
			m_hCharacter->Move(MOVE_BACKWARD);
		else if (flValue > 0.1f)
			m_hCharacter->Move(MOVE_FORWARD);
		else
		{
			m_hCharacter->StopMove(MOVE_BACKWARD);
			m_hCharacter->StopMove(MOVE_FORWARD);
		}
	}
}

void CPlayer::SetCharacter(CCharacter* pCharacter)
{
	m_hCharacter = pCharacter;
	if (pCharacter)
		pCharacter->SetControllingPlayer(this);
}

CCharacter* CPlayer::GetCharacter() const
{
	return m_hCharacter;
}

void CPlayer::SetClient(int iClient)
{
	m_iClient = iClient;
}
