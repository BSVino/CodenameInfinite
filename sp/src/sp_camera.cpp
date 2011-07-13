#include "sp_camera.h"

#include <matrix.h>

#include <tinker/application.h>
#include <tengine/game/game.h>

#include "sp_character.h"
#include "sp_game.h"
#include "planet.h"

Vector CSPCamera::GetCameraPosition()
{
	if (m_bFreeMode)
		return BaseClass::GetCameraPosition();

	CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
	if (!pCharacter)
		return Vector(10,0,0);

	Vector vecEyeHeight = pCharacter->GetUpVector() * pCharacter->EyeHeight();

	return Game()->GetLocalPlayer()->GetCharacter()->GetOrigin() + vecEyeHeight;
}

Vector CSPCamera::GetCameraTarget()
{
	if (m_bFreeMode)
		return BaseClass::GetCameraTarget();

	CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();

	if (!pCharacter)
		return Vector(0,0,0);

	Vector vecEyeHeight = pCharacter->GetUpVector() * pCharacter->EyeHeight();

	return pCharacter->GetOrigin() + vecEyeHeight + Vector(pCharacter->GetTransformation().GetColumn(0));
}

Vector CSPCamera::GetCameraUp()
{
	CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
	if (pCharacter)
		return Vector(pCharacter->GetTransformation().GetColumn(1));

	return BaseClass::GetCameraUp();
}

float CSPCamera::GetCameraFOV()
{
	return 60;
}
