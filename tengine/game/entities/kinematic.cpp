#include "kinematic.h"

#include <physics/physics.h>

REGISTER_ENTITY(CKinematic);

NETVAR_TABLE_BEGIN(CKinematic);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN_EDITOR(CKinematic);
	SAVEDATA_DEFINE_HANDLE(CSaveData::DATA_COPYTYPE, double, m_flLerpTime, "LerpTime");
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, double, m_flLerpStart);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, double, m_flLerpEnd);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, Vector, m_vecLerpStart);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, Vector, m_vecLerpGoal);
	SAVEDATA_DEFINE_HANDLE(CSaveData::DATA_COPYTYPE, double, m_flAngleLerpTime, "AngleLerpTime");
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, double, m_flAngleLerpStart);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, double, m_flAngleLerpEnd);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, EAngle, m_angLerpStart);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, EAngle, m_angLerpGoal);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, bool, m_bLerping);
	SAVEDATA_EDITOR_VARIABLE("DisableBackCulling");
	SAVEDATA_EDITOR_VARIABLE("LerpTime");
	SAVEDATA_EDITOR_VARIABLE("AngleLerpTime");
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CKinematic);
	INPUT_DEFINE(LerpTo);
	INPUT_DEFINE(LerpAnglesTo);
INPUTS_TABLE_END();

void CKinematic::Spawn()
{
	BaseClass::Spawn();

	m_flLerpTime = 1;
	m_flLerpStart = -1;
	m_flAngleLerpTime = 1;
	m_flAngleLerpStart = -1;
}

void CKinematic::OnSetModel()
{
	BaseClass::OnSetModel();

	// In case the model has changed.
	if (IsInPhysics())
		RemoveFromPhysics();

	if (GetModelID() == ~0)
		return;

	AddToPhysics(CT_KINEMATIC);
}

void CKinematic::Think()
{
	BaseClass::Think();

	if (m_flLerpStart > 0)
	{
		double flTime = GameServer()->GetGameTime() - m_flLerpStart;
		float flLerp = RemapVal((float)flTime, 0, (float)(m_flLerpEnd - m_flLerpStart), 0, 1);
		float flRamp = SLerp(flLerp, 0.2f);

		m_bLerping = true;
		SetGlobalOrigin(m_vecLerpStart * (1-flRamp) + m_vecLerpGoal * flRamp);
		m_bLerping = false;

		if (flRamp >= 1)
			m_flLerpStart = -1;
	}

	if (m_flAngleLerpStart > 0)
	{
		double flTime = GameServer()->GetGameTime() - m_flAngleLerpStart;
		float flLerp = RemapVal((float)flTime, 0, (float)(m_flAngleLerpEnd - m_flAngleLerpStart), 0, 1);
		float flRamp = SLerp(flLerp, 0.2f);

		m_bLerping = true;
		SetGlobalAngles(LerpValue(m_angLerpStart, m_angLerpGoal, flRamp));
		m_bLerping = false;

		if (flRamp >= 1)
			m_flAngleLerpStart = -1;
	}
}

void CKinematic::OnSetLocalTransform(TMatrix& m)
{
	BaseClass::OnSetLocalTransform(m);

	if (m_bLerping)
		return;

	m_flLerpStart = -1;
	m_flAngleLerpStart = -1;
}

void CKinematic::LerpTo(const tvector<tstring>& sArgs)
{
	TAssert(sArgs.size() == 3);

	if (sArgs.size() < 3)
	{
		TMsg("Not enough arguments for LerpTo.\n");
		return;
	}

	m_flLerpStart = GameServer()->GetGameTime();
	m_flLerpEnd = m_flLerpStart + m_flLerpTime;

	m_vecLerpStart = GetGlobalOrigin();
	m_vecLerpGoal = Vector((float)stof(sArgs[0]), (float)stof(sArgs[1]), (float)stof(sArgs[2]));
}

void CKinematic::LerpAnglesTo(const tvector<tstring>& sArgs)
{
	TAssert(sArgs.size() == 3);

	if (sArgs.size() < 3)
	{
		TMsg("Not enough arguments for LerpTo.\n");
		return;
	}

	m_flAngleLerpStart = GameServer()->GetGameTime();
	m_flAngleLerpEnd = m_flAngleLerpStart + m_flAngleLerpTime;

	m_angLerpStart = GetGlobalAngles();
	m_angLerpGoal = EAngle((float)stof(sArgs[0]), (float)stof(sArgs[1]), (float)stof(sArgs[2]));
}
