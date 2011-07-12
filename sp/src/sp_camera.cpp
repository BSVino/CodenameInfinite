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

	if (!pCharacter)
		return Vector(0,0,0);

	return pCharacter->GetOrigin() + Vector(pCharacter->GetTransformation().GetColumn(0));

/*	EAngle angLocal = pCharacter->GetAngles();

	Vector vecCharacterUp = pCharacter->GetUpVector();
	Vector vecCharacterForward = Vector(1, 0, 0);
	Vector vecCharacterRight = vecCharacterUp.Cross(vecCharacterForward);
	vecCharacterForward = vecCharacterUp.Cross(vecCharacterRight);

	Vector vecUp = vecCharacterUp * sin(angLocal.p * (M_PI*2 / 360));
	Vector vecForward = vecCharacterForward * cos(angLocal.p * (M_PI*2 / 360));

	return pCharacter->GetOrigin() + vecUp + vecForward;*/
}

Vector CSPCamera::GetCameraUp()
{
	CSPCharacter* pCharacter = SPGame()->GetLocalPlayerCharacter();
	if (pCharacter)
//		return pCharacter->GetUpVector();
		return Vector(pCharacter->GetTransformation().GetColumn(1));

	return BaseClass::GetCameraUp();
}

float CSPCamera::GetCameraFOV()
{
	return 60;
}
