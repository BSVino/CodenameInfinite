#include "sp_camera.h"

#include <matrix.h>

#include <tinker/application.h>
#include <tengine/game/game.h>

#include "sp_character.h"
#include "sp_game.h"
#include "planet.h"
#include "sp_renderer.h"

Vector CSPCamera::GetCameraPosition()
{
	if (m_bFreeMode)
		return BaseClass::GetCameraPosition();

	CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
	if (!pCharacter)
		return Vector(10,0,0);

	Vector vecEyeHeight = pCharacter->GetUpVector() * pCharacter->EyeHeight();

	Vector vecPosition = pCharacter->GetGlobalOrigin() + vecEyeHeight;

	CScalableVector vecReturn(vecPosition, SCALE_METER);

	return vecReturn.GetUnits(SPGame()->GetSPRenderer()->GetRenderingScale());
}

Vector CSPCamera::GetCameraTarget()
{
	if (m_bFreeMode)
		return BaseClass::GetCameraTarget();

	CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();

	if (!pCharacter)
		return Vector(0,0,0);

	return GetCameraPosition() + Vector(pCharacter->GetGlobalTransform().GetColumn(0));
}

Vector CSPCamera::GetCameraUp()
{
	CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
	if (pCharacter)
		return Vector(pCharacter->GetGlobalTransform().GetColumn(1));

	return BaseClass::GetCameraUp();
}

float CSPCamera::GetCameraFOV()
{
	return 60;
}
