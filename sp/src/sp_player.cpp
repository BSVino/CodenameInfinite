#include "sp_player.h"

#include <tengine/game/entities/character.h>
#include <tinker/cvar.h>
#include <tinker/application.h>

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

	Matrix4x4 mRotate;
	mRotate.SetRotation(angMouse);

	Matrix4x4 mTransform = m_hCharacter->GetTransformation();
	mTransform.SetTranslation(Vector(0,0,0));

	Matrix4x4 mNewTransform = mTransform * mRotate;

	mNewTransform.SetTranslation(m_hCharacter->GetOrigin());

	m_hCharacter->SetTransformation(mNewTransform);
}

CSPCharacter* CSPPlayer::GetSPCharacter()
{
	return static_cast<CSPCharacter*>(m_hCharacter.GetPointer());
}
