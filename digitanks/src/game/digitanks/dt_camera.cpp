#include "dt_camera.h"

// This shouldn't be here!
#include <GL/glfw.h>

#include <maths.h>
#include <mtrand.h>
#include <renderer/renderer.h>

#include "digitanks/digitanksgame.h"

#include <ui/digitankswindow.h>
#include <ui/instructor.h>

CDigitanksCamera::CDigitanksCamera()
{
	m_flTargetRamp = m_flDistanceRamp = m_flAngleRamp = 0;
	m_angCamera = EAngle(45, 0, 0);
	m_bRotatingCamera = false;
	m_flShakeMagnitude = 0;
}

void CDigitanksCamera::SetTarget(Vector vecTarget)
{
	if (vecTarget.x < -DigitanksGame()->GetTerrain()->GetMapSize())
		vecTarget.x = -DigitanksGame()->GetTerrain()->GetMapSize();
	if (vecTarget.z < -DigitanksGame()->GetTerrain()->GetMapSize())
		vecTarget.z = -DigitanksGame()->GetTerrain()->GetMapSize();
	if (vecTarget.x > DigitanksGame()->GetTerrain()->GetMapSize())
		vecTarget.x = DigitanksGame()->GetTerrain()->GetMapSize();
	if (vecTarget.z > DigitanksGame()->GetTerrain()->GetMapSize())
		vecTarget.z = DigitanksGame()->GetTerrain()->GetMapSize();

	DigitanksGame()->GetTerrain()->SetPointHeight(vecTarget);

	m_flTargetRamp = Game()->GetGameTime();
	m_vecOldTarget = m_vecTarget;
	m_vecNewTarget = vecTarget;
	m_vecVelocity = m_vecGoalVelocity = Vector(0,0,0);
}

void CDigitanksCamera::SnapTarget(Vector vecTarget)
{
	DigitanksGame()->GetTerrain()->SetPointHeight(vecTarget);

	m_flTargetRamp = 0;
	m_vecNewTarget = m_vecTarget = vecTarget;
	m_vecVelocity = m_vecGoalVelocity = Vector(0,0,0);
}

void CDigitanksCamera::SetDistance(float flDistance)
{
	if (flDistance < 40)
		flDistance = 40;

	if (flDistance > 300)
		flDistance = 300;

	m_flDistanceRamp = Game()->GetGameTime();
	m_flOldDistance = m_flDistance;
	m_flNewDistance = flDistance;
}

void CDigitanksCamera::SnapDistance(float flDistance)
{
	m_flDistanceRamp = 0;
	m_flNewDistance = flDistance;
}

void CDigitanksCamera::SetAngle(EAngle angCamera)
{
	m_flAngleRamp = Game()->GetGameTime();
	m_angOldAngle = m_angCamera;
	m_angNewAngle = angCamera;
}

void CDigitanksCamera::SnapAngle(EAngle angCamera)
{
	m_angCamera = angCamera;
}

void CDigitanksCamera::ZoomOut()
{
	SetDistance(m_flNewDistance+20);

	CDigitanksWindow::Get()->GetInstructor()->FinishedTutorial(CInstructor::TUTORIAL_ZOOMCAMERA);
}

void CDigitanksCamera::ZoomIn()
{
	SetDistance(m_flNewDistance-20);

	CDigitanksWindow::Get()->GetInstructor()->FinishedTutorial(CInstructor::TUTORIAL_ZOOMCAMERA);
}

void CDigitanksCamera::Shake(Vector vecLocation, float flMagnitude)
{
	m_vecShakeLocation = vecLocation;
	m_flShakeMagnitude = flMagnitude;
}

void CDigitanksCamera::Think()
{
	BaseClass::Think();

	float flGameTime = Game()->GetGameTime();
	float flFrameTime = Game()->GetFrameTime();
	float flLerpTime = 0.5f;
	float flLerpAmount = 0.7f;

	m_vecVelocity.x = Approach(m_vecGoalVelocity.x, m_vecVelocity.x, flFrameTime*200);
	m_vecVelocity.z = Approach(m_vecGoalVelocity.z, m_vecVelocity.z, flFrameTime*200);

	if (m_flTargetRamp && flGameTime - m_flTargetRamp < flLerpTime)
	{
		float flLerp = Lerp((flGameTime - m_flTargetRamp)/flLerpTime, flLerpAmount);
		m_vecTarget = m_vecNewTarget * flLerp + m_vecOldTarget * (1-flLerp);
		m_vecVelocity = m_vecGoalVelocity = Vector(0,0,0);
	}
	else
	{
		if (m_flTargetRamp)
		{
			m_vecTarget = m_vecNewTarget;
			m_flTargetRamp = 0;
		}
		else
		{
			Matrix4x4 mRotation;
			mRotation.SetRotation(EAngle(0, m_angCamera.y, 0));
			Vector vecVelocity = mRotation * m_vecVelocity;

			m_vecTarget = m_vecTarget + vecVelocity * flFrameTime;

			if (m_vecTarget.x < -DigitanksGame()->GetTerrain()->GetMapSize())
				m_vecTarget.x = -DigitanksGame()->GetTerrain()->GetMapSize();
			if (m_vecTarget.z < -DigitanksGame()->GetTerrain()->GetMapSize())
				m_vecTarget.z = -DigitanksGame()->GetTerrain()->GetMapSize();
			if (m_vecTarget.x > DigitanksGame()->GetTerrain()->GetMapSize())
				m_vecTarget.x = DigitanksGame()->GetTerrain()->GetMapSize();
			if (m_vecTarget.z > DigitanksGame()->GetTerrain()->GetMapSize())
				m_vecTarget.z = DigitanksGame()->GetTerrain()->GetMapSize();

			DigitanksGame()->GetTerrain()->SetPointHeight(m_vecTarget);
		}
	}

	if (m_flDistanceRamp && flGameTime - m_flDistanceRamp < flLerpTime)
	{
		float flLerp = Lerp((flGameTime - m_flDistanceRamp)/flLerpTime, flLerpAmount);
		m_flDistance = m_flNewDistance * flLerp + m_flOldDistance * (1-flLerp);
	}
	else
		m_flDistance = m_flNewDistance;

	if (m_flAngleRamp && flGameTime - m_flAngleRamp < flLerpTime)
	{
		float flLerp = Lerp((flGameTime - m_flAngleRamp)/flLerpTime, flLerpAmount);
		float flPitch = m_angOldAngle.p + AngleDifference(m_angNewAngle.p, m_angOldAngle.p) * flLerp;
		float flYaw = m_angOldAngle.y + AngleDifference(m_angNewAngle.y, m_angOldAngle.y) * flLerp;
		float flRoll = m_angOldAngle.r + AngleDifference(m_angNewAngle.r, m_angOldAngle.r) * flLerp;
		m_angCamera = EAngle(flPitch, flYaw, flRoll);
	}

	m_vecCamera = AngleVector(m_angCamera) * m_flDistance + m_vecTarget;

	m_flShakeMagnitude = Approach(0, m_flShakeMagnitude, Game()->GetFrameTime()*5);
	if (m_flShakeMagnitude)
	{
		Vector vecRight, vecUp;
		Game()->GetRenderer()->GetCameraVectors(NULL, &vecRight, &vecUp);

		float flX = RandomFloat(-m_flShakeMagnitude, m_flShakeMagnitude);
		float flY = RandomFloat(-m_flShakeMagnitude, m_flShakeMagnitude);
		m_vecShake = vecRight * flX + vecUp * flY;

		float flDistance = (m_vecTarget-m_vecShakeLocation).Length();
		float flScale = 1;
		if (flDistance > 50)
			flScale = RemapValClamped(flDistance, 50, 200, 1, 0);
		m_vecShake *= flScale;
	}
	else
		m_vecShake = Vector(0,0,0);
}

Vector CDigitanksCamera::GetCameraPosition()
{
	if (m_bFreeMode)
		return BaseClass::GetCameraPosition();

	return m_vecCamera + m_vecShake;
}

Vector CDigitanksCamera::GetCameraTarget()
{
	if (m_bFreeMode)
		return BaseClass::GetCameraTarget();

	return m_vecTarget + m_vecShake;
}

void CDigitanksCamera::MouseInput(int x, int y)
{
	if (m_bRotatingCamera)
	{
		m_angCamera.y += (x/5.0f);
		m_angCamera.p += (y/5.0f);

		if (m_angCamera.p > 89)
			m_angCamera.p = 89;

		if (m_angCamera.p < 20)
			m_angCamera.p = 20;

		while (m_angCamera.y > 180)
			m_angCamera.y -= 360;

		while (m_angCamera.y < -180)
			m_angCamera.y += 360;
	}

	BaseClass::MouseInput(x, y);
}

void CDigitanksCamera::MouseButton(int iButton, int iState)
{
	if (iButton == GLFW_MOUSE_BUTTON_2)
		m_bRotatingCamera = !!iState;

	BaseClass::MouseButton(iButton, iState);
}

void CDigitanksCamera::KeyDown(int c)
{
	if (!m_bFreeMode)
	{
		if (c == GLFW_KEY_UP)
			m_vecGoalVelocity.x = -80.0f;
		if (c == GLFW_KEY_DOWN)
			m_vecGoalVelocity.x = 80.0f;
		if (c == GLFW_KEY_RIGHT)
			m_vecGoalVelocity.z = -80.0f;
		if (c == GLFW_KEY_LEFT)
			m_vecGoalVelocity.z = 80.0f;
	}

	BaseClass::KeyDown(c);
}

void CDigitanksCamera::KeyUp(int c)
{
	if (!m_bFreeMode)
	{
		if (c == GLFW_KEY_UP)
			m_vecGoalVelocity.x = 0.0f;
		if (c == GLFW_KEY_DOWN)
			m_vecGoalVelocity.x = 0.0f;
		if (c == GLFW_KEY_RIGHT)
			m_vecGoalVelocity.z = 0.0f;
		if (c == GLFW_KEY_LEFT)
			m_vecGoalVelocity.z = 0.0f;
	}

	BaseClass::KeyUp(c);
}