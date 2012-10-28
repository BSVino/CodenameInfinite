#include "charactercamera.h"

#include <tinker/cvar.h>
#include <game/entities/character.h>

REGISTER_ENTITY(CCharacterCamera);

NETVAR_TABLE_BEGIN(CCharacterCamera);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CCharacterCamera);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, bool, m_bThirdPerson);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CEntityHandle<CCharacter>, m_hCharacter);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, float, m_flBack);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, float, m_flUp);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, float, m_flSide);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CCharacterCamera);
INPUTS_TABLE_END();

CCharacterCamera::CCharacterCamera()
{
	m_bThirdPerson = false;
	m_flBack = 1;
	m_flUp = 1;
	m_flSide = 1;
}

void CCharacterCamera::CameraThink()
{
	// This whole entity is untested since the move to the camera manager.
	TUnimplemented();

	BaseClass::CameraThink();

	CCharacter* pCharacter = m_hCharacter;
	if (!pCharacter)
		return;

	if (GetThirdPerson())
	{
		SetGlobalOrigin(GetThirdPersonCameraPosition());
		SetGlobalAngles(VectorAngles(GetThirdPersonCameraDirection()));
	}
	else
	{
		TUnimplemented();
		SetGlobalOrigin(pCharacter->GetGlobalOrigin() + pCharacter->GetUpVector() * (TFloat)pCharacter->EyeHeight());
		SetGlobalAngles(pCharacter->GetViewAngles());
	}
}

TVector CCharacterCamera::GetThirdPersonCameraPosition()
{
	CCharacter* pCharacter = m_hCharacter;
	if (!pCharacter)
		return TVector(10, 0, 0);

	TVector vecEyeHeight = pCharacter->GetUpVector() * pCharacter->EyeHeight();

	TMatrix mView = TMatrix(pCharacter->GetThirdPersonCameraAngles(), TVector());
	TVector vecThird = pCharacter->GetGlobalTransform().GetTranslation() + vecEyeHeight;
	vecThird -= Vector(mView.GetForwardVector()) * m_flBack;
	vecThird += Vector(mView.GetRightVector()) * m_flSide;
	vecThird += Vector(mView.GetUpVector()) * m_flUp;

	return vecThird;
}

TVector CCharacterCamera::GetThirdPersonCameraDirection()
{
	CCharacter* pCharacter = m_hCharacter;

	if (!pCharacter)
		return TVector();

	return AngleVector(pCharacter->GetThirdPersonCameraAngles());
}
