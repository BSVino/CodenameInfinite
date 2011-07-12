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

	return Game()->GetLocalPlayer()->GetCharacter()->GetOrigin();
}

Vector CSPCamera::GetCameraTarget()
{
	if (m_bFreeMode)
		return BaseClass::GetCameraTarget();

	CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();

	Vector vecDirection = AngleVector(pCharacter->GetAngles());

	return pCharacter->GetOrigin() + vecDirection;
}

Vector CSPCamera::GetCameraUp()
{
	return BaseClass::GetCameraUp();
}

float CSPCamera::GetCameraFOV()
{
	return 60;
}
