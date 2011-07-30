#include "sp_camera.h"

#include <matrix.h>

#include <tinker/application.h>
#include <tengine/game/game.h>

#include "sp_character.h"
#include "sp_game.h"
#include "planet.h"
#include "sp_renderer.h"

CScalableVector CSPCamera::GetCameraScalablePosition()
{
	if (m_bFreeMode)
		return CScalableVector(BaseClass::GetCameraPosition(), SCALE_METER);

	CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
	if (!pCharacter)
		return CScalableVector(Vector(10,0,0), SCALE_METER);

	CScalableVector vecEyeHeight = pCharacter->GetUpVector() * pCharacter->EyeHeightScalable();

	return vecEyeHeight;
}

CScalableVector CSPCamera::GetCameraScalableTarget()
{
	if (m_bFreeMode)
		return CScalableVector(BaseClass::GetCameraTarget(), SCALE_METER);

	CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();

	if (!pCharacter)
		return CScalableVector();

	return GetCameraScalablePosition() + CScalableVector(pCharacter->GetGlobalScalableTransform().GetForwardVector(), SPGame()->GetSPRenderer()->GetRenderingScale());
}

Vector CSPCamera::GetCameraPosition()
{
	scale_t eScale = SPGame()->GetSPRenderer()->GetRenderingScale();
	if (eScale == SCALE_NONE)
		return Vector(0, 0, 0);

	return GetCameraScalablePosition().GetUnits(eScale);
}

Vector CSPCamera::GetCameraTarget()
{
	scale_t eScale = SPGame()->GetSPRenderer()->GetRenderingScale();
	if (eScale == SCALE_NONE)
		return Vector(10, 0, 0);

	return GetCameraScalableTarget().GetUnits(eScale);
}

Vector CSPCamera::GetCameraUp()
{
	CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
	if (pCharacter)
		return Vector(pCharacter->GetGlobalScalableTransform().GetUpVector());

	return BaseClass::GetCameraUp();
}

float CSPCamera::GetCameraFOV()
{
	return 60;
}
