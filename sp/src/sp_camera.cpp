#include "sp_camera.h"

#include <matrix.h>

#include <tinker/application.h>
#include <tengine/game/game.h>

#include "sp_character.h"
#include "sp_game.h"
#include "planet.h"
#include "sp_renderer.h"

CScalableVector CSPCamera::GetCameraPosition()
{
	if (m_bFreeMode)
		return CScalableVector(BaseClass::GetCameraPosition(), SCALE_METER);

	CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
	if (!pCharacter)
		return CScalableVector(Vector(10,0,0), SCALE_METER);

	CScalableVector vecEyeHeight = pCharacter->GetUpVector() * pCharacter->EyeHeight();

	return vecEyeHeight;
}

CScalableVector CSPCamera::GetCameraTarget()
{
	if (m_bFreeMode)
		return CScalableVector(BaseClass::GetCameraTarget(), SCALE_METER);

	CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();

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
	CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
	if (pCharacter)
		return Vector(pCharacter->GetGlobalTransform().GetUpVector());

	return BaseClass::GetCameraUp();
}

float CSPCamera::GetCameraFOV()
{
	return 60;
}
