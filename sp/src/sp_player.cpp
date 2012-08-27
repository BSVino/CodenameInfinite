#include "sp_player.h"

#include <tengine/game/entities/character.h>
#include <tinker/cvar.h>
#include <tinker/application.h>
#include <tinker/keys.h>

#include "sp_playercharacter.h"

REGISTER_ENTITY(CSPPlayer);

NETVAR_TABLE_BEGIN(CSPPlayer);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CSPPlayer);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CSPPlayer);
INPUTS_TABLE_END();

void CSPPlayer::MouseMotion(int x, int y)
{
	if (m_hCharacter == NULL)
		return;

	float flSensitivity = CVar::GetCVarFloat("m_sensitivity");
	float flYaw = (x*flSensitivity/20);
	float flPitch = (y*flSensitivity/20);

	EAngle angMouse;
	angMouse.p = -flPitch;
	angMouse.y = flYaw;
	angMouse.r = 0;

	CScalableMatrix mRotate;
	mRotate.SetAngles(angMouse);

	CScalableMatrix mTransform = GetPlayerCharacter()->GetLocalTransform();
	mTransform.SetTranslation(CScalableVector());

	CScalableMatrix mNewTransform = mTransform * mRotate;

	mNewTransform.SetTranslation(GetPlayerCharacter()->GetLocalOrigin());

	mNewTransform.SetForwardVector(mNewTransform.GetForwardVector().Normalized());
	mNewTransform.SetRightVector(mNewTransform.GetForwardVector().Cross(mNewTransform.GetUpVector()).Normalized());
	mNewTransform.SetUpVector(mNewTransform.GetRightVector().Cross(mNewTransform.GetForwardVector()));

	GetPlayerCharacter()->SetLocalTransform(mNewTransform);
}

void CSPPlayer::KeyPress(int c)
{
	BaseClass::KeyPress(c);

	if (GetPlayerCharacter() == NULL)
		return;

	if (c == TINKER_KEY_LSHIFT)
		GetPlayerCharacter()->EngageHyperdrive();

	if (c == TINKER_KEY_LCTRL)
		GetPlayerCharacter()->SetWalkSpeedOverride(true);

	if (c == 'F')
		GetPlayerCharacter()->ToggleFlying();
}

void CSPPlayer::KeyRelease(int c)
{
	BaseClass::KeyRelease(c);

	if (GetPlayerCharacter() == NULL)
		return;

	if (c == TINKER_KEY_LSHIFT)
		GetPlayerCharacter()->DisengageHyperdrive();

	if (c == TINKER_KEY_LCTRL)
		GetPlayerCharacter()->SetWalkSpeedOverride(false);
}

CPlayerCharacter* CSPPlayer::GetPlayerCharacter()
{
	return static_cast<CPlayerCharacter*>(m_hCharacter.GetPointer());
}
