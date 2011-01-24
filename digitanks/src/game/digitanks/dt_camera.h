#ifndef DT_DIGITANKS_CAMERA_H
#define DT_DIGITANKS_CAMERA_H

#include <camera.h>
#include <common.h>

#include <game/digitanks/weapons/cameraguided.h>

class CDigitanksCamera : public CCamera
{
	DECLARE_CLASS(CDigitanksCamera, CCamera);

public:
					CDigitanksCamera();

public:
	void			SetTarget(Vector vecTarget);
	void			SnapTarget(Vector vecTarget);
	void			SetDistance(float flDistance);
	void			SnapDistance(float flDistance);
	void			SetAngle(EAngle angCamera);
	void			SnapAngle(EAngle angCamera);

	EAngle			GetAngles() { return m_angCamera; }

	void			ZoomOut();
	void			ZoomIn();

	void			Shake(Vector vecLocation, float flMagnitude);

	void			SetCameraGuidedMissile(class CCameraGuidedMissile* pMissile);

	virtual void	Think();

	virtual Vector	GetCameraPosition();
	virtual Vector	GetCameraTarget();
	virtual float	GetCameraFOV();
	virtual float	GetCameraNear();
	virtual float	GetCameraFar();

	virtual void	SetFreeMode(bool bOn);

	virtual void	MouseInput(int x, int y);
	virtual void	MouseButton(int iButton, int iState);
	virtual void	KeyDown(int c);
	virtual void	KeyUp(int c);

public:
	Vector			m_vecOldTarget;
	Vector			m_vecNewTarget;
	float			m_flTargetRamp;

	float			m_flOldDistance;
	float			m_flNewDistance;
	float			m_flDistanceRamp;

	EAngle			m_angOldAngle;
	EAngle			m_angNewAngle;
	float			m_flAngleRamp;

	Vector			m_vecShakeLocation;
	float			m_flShakeMagnitude;
	Vector			m_vecShake;

	Vector			m_vecTarget;
	float			m_flDistance;

	Vector			m_vecCamera;
	EAngle			m_angCamera;

	Vector			m_vecVelocity;
	Vector			m_vecGoalVelocity;

	bool			m_bRotatingCamera;

	bool			m_bMouseDragLeft;
	bool			m_bMouseDragRight;
	bool			m_bMouseDragUp;
	bool			m_bMouseDragDown;

	CEntityHandle<class CCameraGuidedMissile>	m_hCameraGuidedMissile;
	float			m_flCameraGuidedFOV;
	float			m_flCameraGuidedFOVGoal;
};

#endif
