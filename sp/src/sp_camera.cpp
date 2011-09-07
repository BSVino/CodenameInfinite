#include "sp_camera.h"

#include <matrix.h>

#include <tinker/application.h>
#include <tengine/game/game.h>
#include <tinker/cvar.h>

#include "sp_playercharacter.h"
#include "sp_game.h"
#include "planet.h"
#include "sp_renderer.h"

CSPCamera::CSPCamera()
{
	m_bThirdPerson = false;
}

CScalableVector CSPCamera::GetCameraPosition()
{
	if (m_bFreeMode)
		return CScalableVector(BaseClass::GetCameraPosition(), SCALE_METER);

	if (GetThirdPerson())
		return GetThirdPersonCameraPosition();

	CPlayerCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
	if (!pCharacter)
		return CScalableVector(Vector(10,0,0), SCALE_METER);

	// Freeze transforms
	pCharacter->GetGlobalTransform();

	CScalableVector vecEyeHeight = pCharacter->GetUpVector() * pCharacter->EyeHeight();

	return vecEyeHeight;
}

CScalableVector CSPCamera::GetCameraTarget()
{
	if (m_bFreeMode)
		return CScalableVector(BaseClass::GetCameraTarget(), SCALE_METER);

	if (GetThirdPerson())
		return GetThirdPersonCameraTarget();

	CPlayerCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();

	if (!pCharacter)
		return CScalableVector();

	scale_t eScale = SPGame()->GetSPRenderer()->GetRenderingScale();

	// Make sure we don't lose data when working in millimeters
	if (eScale < SCALE_METER)
		eScale = SCALE_METER;

	return GetCameraPosition() + CScalableVector(pCharacter->GetGlobalTransform().GetForwardVector(), eScale);
}

TVector CSPCamera::GetCameraUp()
{
	CPlayerCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
	if (pCharacter)
		return Vector(pCharacter->GetGlobalTransform().GetUpVector());

	return BaseClass::GetCameraUp();
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
	CPlayerCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
	if (!pCharacter)
		return CScalableVector(Vector(10,0,0), SCALE_METER);

	// Freeze
	pCharacter->GetGlobalTransform();

	CScalableVector vecEyeHeight = pCharacter->GetUpVector() * pCharacter->EyeHeight();

	CScalableVector vecThird = vecEyeHeight - pCharacter->GetGlobalTransform().GetForwardVector() * cam_third_back.GetFloat();
	vecThird += pCharacter->GetGlobalTransform().GetRightVector() * cam_third_right.GetFloat();

	return vecThird;
}

CScalableVector CSPCamera::GetThirdPersonCameraTarget()
{
	CPlayerCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();

	if (!pCharacter)
		return CScalableVector();

	CScalableVector vecEyeHeight = pCharacter->GetUpVector() * pCharacter->EyeHeight();
	vecEyeHeight += pCharacter->GetGlobalTransform().GetRightVector() * cam_third_right.GetFloat();

	return vecEyeHeight;
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
