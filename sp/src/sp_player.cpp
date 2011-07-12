#include "sp_player.h"

#include <tengine/game/entities/character.h>
#include <tinker/cvar.h>
#include <tinker/application.h>

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

/*
void CSPPlayer::MouseMotion(int x, int y)
{
	if (m_hCharacter == NULL)
		return;

	Quaternion qRotation = m_hCharacter->GetRotation();

	float flSensitivity = CVar::GetCVarFloat("m_sensitivity")*40;
	float flYaw = (x/flSensitivity);
	float flPitch = (y/flSensitivity);

	if (x != 0)
	{
		Quaternion qYaw;
		qYaw.SetRotation(-flYaw, m_hCharacter->GetUpVector());

		qRotation *= qYaw;
	}

	if (y != 0)
	{
		Vector vecUp = m_hCharacter->GetUpVector();
		Matrix4x4 mQuat(qRotation);
		Vector vecForward = Vector(mQuat.GetColumn(0)).Normalized();

		// Make absolutely sure the up and forward vector aren't pointing in the same direction.
		if (fabs(vecUp.Dot(vecForward)) > 0.99f)
		{
			vecUp = Vector(1, 0, 0);
			if (fabs(vecUp.Dot(vecForward)) > 0.99f)
				vecUp = Vector(0, 0, 1);
		}

		Vector vecRight = vecForward.Cross(vecUp).Normalized();

		Quaternion qPitch;
		qPitch.SetRotation(-flPitch, vecRight);

		qRotation *= qPitch;
	}

	qRotation.Normalize();

	m_hCharacter->SetRotation(qRotation);
}*/

/*void CSPPlayer::MouseMotion(int x, int y)
{
	if (m_hCharacter == NULL)
		return;

	Matrix4x4 mGlobal = m_hCharacter->GetTransformation();
	mGlobal.SetTranslation(Vector(0,0,0));

	// Create a local frame of reference so we can grab some eulers.
	Vector vecRootUp = m_hCharacter->GetUpVector();
	Vector vecRootForward = AngleVector(m_hCharacter->GetAngles());

	// Make absolutely sure the up and forward vector aren't pointing in the same direction.
	if (fabs(vecRootUp.Dot(vecRootForward)) > 0.99f)
	{
		vecRootForward = Vector(1, 0, 0);
		if (fabs(vecRootUp.Dot(vecRootForward)) > 0.99f)
			vecRootForward = Vector(0, 0, 1);
	}

	Vector vecRootRight	= vecRootForward.Cross(vecRootUp).Normalized();
	vecRootForward = vecRootUp.Cross(vecRootRight).Normalized();

	Matrix4x4 mRoot(vecRootForward, vecRootUp, vecRootRight);
	Matrix4x4 mRootInverse = mRoot;
	mRootInverse.InvertTR();

	Matrix4x4 mLocal = mGlobal * mRootInverse;

	EAngle angDirection = mLocal.GetAngles();

	float flYaw = (x/CVar::GetCVarFloat("m_sensitivity"));
	float flPitch = (y/CVar::GetCVarFloat("m_sensitivity"));

	if (angDirection.p + flPitch > 89)
		flPitch = 89 - angDirection.p;
	if (angDirection.p - flPitch < -89)
		flPitch = -89 - angDirection.p;

	static float flLast = 0;
	if (GameServer()->GetGameTime() - flLast > 0.5f)
	{
//		TMsg(sprintf("%f %f %f %f\n", GameServer()->GetGameTime(), angDirection.p, angDirection.y, angDirection.r));
		TMsg(sprintf("%f Up: %f %f %f Right: %f %f %f\n", GameServer()->GetGameTime(), vecRootUp.x, vecRootUp.y, vecRootUp.z, vecRootRight.x, vecRootRight.y, vecRootRight.z));
		flLast = GameServer()->GetGameTime();
	}

	// Rebuild our global matrix from scratch so we can rotate it. I was getting gimbal problems so I'm doing this.
	Vector vecGlobalUp = m_hCharacter->GetUpVector();
	Vector vecGlobalForward = Vector(mGlobal.GetColumn(0)).Normalized();

	// Make absolutely sure the up and forward vector aren't pointing in the same direction.
	if (fabs(vecGlobalUp.Dot(vecGlobalForward)) > 0.99f)
	{
		vecGlobalUp = Vector(1, 0, 0);
		if (fabs(vecGlobalUp.Dot(vecGlobalForward)) > 0.99f)
			vecGlobalUp = Vector(0, 0, 1);
	}

	Vector vecGlobalRight = vecGlobalForward.Cross(vecGlobalUp).Normalized();
	vecGlobalUp = vecGlobalRight.Cross(vecGlobalForward).Normalized();

	Matrix4x4 mNewGlobal(vecGlobalForward, vecGlobalUp, vecGlobalRight);

	static float flLast2 = 0;
	if (GameServer()->GetGameTime() - flLast2 > 0.2f)
	{
		TMsg(sprintf("%f\nFd: %f %f %f\nUp: %f %f %f\nRt: %f %f %f\n", GameServer()->GetGameTime(),
			vecGlobalForward.x, vecGlobalForward.y, vecGlobalForward.z,
			vecGlobalUp.x, vecGlobalUp.y, vecGlobalUp.z,
			vecGlobalRight.x, vecGlobalRight.y, vecGlobalRight.z));
		flLast2 = GameServer()->GetGameTime();
	}

	Matrix4x4 mYaw;
	mYaw.SetRotation(-flYaw, vecRootUp);

	Matrix4x4 mPitch;
	mPitch.SetRotation(-flPitch, vecGlobalRight);

	mNewGlobal = mNewGlobal * mPitch;
//	mNewGlobal = mNewGlobal * mYaw;

	mNewGlobal.SetColumn(0, Vector(mNewGlobal.GetColumn(0)).Normalized());
	mNewGlobal.SetColumn(1, Vector(mNewGlobal.GetColumn(1)).Normalized());
	mNewGlobal.SetColumn(2, Vector(mNewGlobal.GetColumn(2)).Normalized());

	mNewGlobal.SetTranslation(m_hCharacter->GetOrigin());
	m_hCharacter->SetTransformation(mNewGlobal);
}
*/