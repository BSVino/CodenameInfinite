#include "sp_player.h"

#include <tengine/game/entities/character.h>
#include <tinker/cvar.h>
#include <tinker/application.h>
#include <tinker/keys.h>

#include "sp_character.h"

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
	float flYaw = (x/flSensitivity);
	float flPitch = (y/flSensitivity);

	EAngle angMouse;
	angMouse.p = -flPitch;
	angMouse.y = -flYaw;
	angMouse.r = 0;

	CScalableMatrix mRotate;
	mRotate.SetAngles(angMouse);

	CScalableMatrix mTransform = GetSPCharacter()->GetLocalScalableTransform();
	mTransform.SetTranslation(CScalableVector());

	CScalableMatrix mNewTransform = mTransform * mRotate;

	mNewTransform.SetTranslation(GetSPCharacter()->GetLocalScalableOrigin());

	GetSPCharacter()->SetLocalScalableTransform(mNewTransform);
}

void CSPPlayer::KeyPress(int c)
{
	BaseClass::KeyPress(c);

	if (GetSPCharacter() == NULL)
		return;

	if (c == TINKER_KEY_LSHIFT)
		GetSPCharacter()->EngageHyperdrive();
}

void CSPPlayer::KeyRelease(int c)
{
	BaseClass::KeyRelease(c);

	if (GetSPCharacter() == NULL)
		return;

	if (c == TINKER_KEY_LSHIFT)
		GetSPCharacter()->DisengageHyperdrive();
}

CSPCharacter* CSPPlayer::GetSPCharacter()
{
	return static_cast<CSPCharacter*>(m_hCharacter.GetPointer());
}
