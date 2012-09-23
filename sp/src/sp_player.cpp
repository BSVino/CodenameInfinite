#include "sp_player.h"

#include <tengine/game/entities/character.h>
#include <tinker/cvar.h>
#include <tinker/application.h>
#include <tinker/keys.h>

#include "sp_playercharacter.h"
#include "planet.h"

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

	if (GetPlayerCharacter()->GetNearestPlanet())
	{
		Matrix4x4 mPlanetLocalRotation(GetPlayerCharacter()->GetViewAngles());

		// Construct a "local space" for the surface
		Vector vecSurfaceUp = GetPlayerCharacter()->GetLocalUpVector();
		Vector vecSurfaceForward(1, 0, 0);
		Vector vecSurfaceRight = vecSurfaceForward.Cross(vecSurfaceUp).Normalized();
		vecSurfaceForward = vecSurfaceUp.Cross(vecSurfaceRight).Normalized();

		Matrix4x4 mSurface(vecSurfaceForward, vecSurfaceUp, vecSurfaceRight);
		Matrix4x4 mSurfaceInverse = mSurface;
		mSurfaceInverse.InvertRT();

		// Bring our current view angles into that local space
		Matrix4x4 mLocalRotation = mSurfaceInverse * mPlanetLocalRotation;
		EAngle angLocalRotation = mLocalRotation.GetAngles();

		angLocalRotation.y += flYaw;
		angLocalRotation.p -= flPitch;
		// Don't lock this here since it will snap as we're rolling in from space. LockViewToPlanet() keeps us level.
		//angLocalRotation.r = 0;

		if (angLocalRotation.p > 89)
			angLocalRotation.p = 89;

		if (angLocalRotation.p < -89)
			angLocalRotation.p = -89;

		while (angLocalRotation.y > 180)
			angLocalRotation.y -= 360;

		while (angLocalRotation.y < -180)
			angLocalRotation.y += 360;

		Matrix4x4 mNewLocalRotation;
		mNewLocalRotation.SetAngles(angLocalRotation);

		Matrix4x4 mNewGlobalRotation = mSurface * mNewLocalRotation;

		GetPlayerCharacter()->SetViewAngles(mNewGlobalRotation.GetAngles());
	}
	else
	{
		EAngle angMouse;
		angMouse.p = -flPitch;
		angMouse.y = flYaw;
		angMouse.r = 0;

		CScalableMatrix mRotate;
		mRotate.SetAngles(angMouse);

		CScalableMatrix mTransform(GetPlayerCharacter()->GetViewAngles());

		CScalableMatrix mNewTransform = mTransform * mRotate;

		GetPlayerCharacter()->SetViewAngles(mNewTransform.GetAngles());
	}
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
