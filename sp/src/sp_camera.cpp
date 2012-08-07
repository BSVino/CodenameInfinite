#include "sp_camera.h"

#include <matrix.h>

#include <tinker/application.h>
#include <tengine/game/entities/game.h>
#include <tinker/cvar.h>

#include "sp_character.h"
#include "sp_game.h"
#include "planet.h"
#include "sp_renderer.h"

REGISTER_ENTITY(CSPCamera);

NETVAR_TABLE_BEGIN(CSPCamera);
NETVAR_TABLE_END();

SAVEDATA_TABLE_BEGIN(CSPCamera);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, bool, m_bThirdPerson);
	SAVEDATA_DEFINE(CSaveData::DATA_COPYTYPE, CSPCharacter, m_hCharacter);
SAVEDATA_TABLE_END();

INPUTS_TABLE_BEGIN(CSPCamera);
INPUTS_TABLE_END();

CSPCamera::CSPCamera()
{
	m_bThirdPerson = false;
}

void CSPCamera::CameraThink()
{
	CSPCharacter* pCharacter = m_hCharacter;
	if (!pCharacter)
		return;

	if (GetThirdPerson())
	{
		SetGlobalOrigin(GetThirdPersonCameraPosition());
		SetGlobalAngles(VectorAngles(GetThirdPersonCameraDirection()));
		return;
	}

	// Freeze transforms
	pCharacter->GetGlobalTransform();

	CScalableVector vecEyeHeight = pCharacter->GetUpVector() * pCharacter->EyeHeight();

	SetGlobalOrigin(vecEyeHeight);

	SetGlobalAngles(pCharacter->GetGlobalAngles());
}

const TVector CSPCamera::GetUpVector() const
{
	CSPCharacter* pCharacter = m_hCharacter;
	if (pCharacter)
		return Vector(pCharacter->GetGlobalTransform().GetUpVector());

	return BaseClass::GetUpVector();
}

float CSPCamera::GetCameraFOV()
{
	return 60;
}

float CSPCamera::GetCameraNear()
{
	if (SPGame()->GetSPRenderer()->GetRenderingScale() == SCALE_RENDER)
		return 0.05f;

	return 1;
}

float CSPCamera::GetCameraFar()
{
	if (SPGame()->GetSPRenderer()->GetRenderingScale() == SCALE_RENDER)
		return 500;

	return 1200;
}

CVar cam_third_back("cam_third_back", "1");
CVar cam_third_right("cam_third_right", "0.2");

CScalableVector CSPCamera::GetThirdPersonCameraPosition()
{
	CSPCharacter* pCharacter = m_hCharacter;
	if (!pCharacter)
		return CScalableVector(Vector(10,0,0), SCALE_METER);

	// Freeze
	pCharacter->GetGlobalTransform();

	CScalableVector vecEyeHeight = pCharacter->GetUpVector() * pCharacter->EyeHeight();

	CScalableVector vecThird = vecEyeHeight - pCharacter->GetGlobalTransform().GetForwardVector() * cam_third_back.GetFloat();
	vecThird += pCharacter->GetGlobalTransform().GetRightVector() * cam_third_right.GetFloat();

	return vecThird;
}

Vector CSPCamera::GetThirdPersonCameraDirection()
{
	CSPCharacter* pCharacter = m_hCharacter;

	if (!pCharacter)
		return Vector();

	CScalableVector vecEyeHeight = pCharacter->GetUpVector() * pCharacter->EyeHeight();
	vecEyeHeight += pCharacter->GetGlobalTransform().GetRightVector() * cam_third_right.GetFloat();

	return (vecEyeHeight - GetThirdPersonCameraPosition()).GetUnits(SCALE_METER);
}

void CSPCamera::KeyDown(int c)
{
	if (c == 'X')
		ToggleThirdPerson();
}

void CSPCamera::ToggleThirdPerson()
{
	SetThirdPerson(!GetThirdPerson());
}

void CSPCamera::SetCharacter(CSPCharacter* pCharacter)
{
	m_hCharacter = pCharacter;
}
