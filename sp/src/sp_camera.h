#ifndef SP_CAMERA_H
#define SP_CAMERA_H

#include <game/camera.h>
#include <common.h>

#include "sp_common.h"

class CSPCamera : public CCamera
{
	DECLARE_CLASS(CSPCamera, CCamera);

public:
								CSPCamera();

public:
	virtual CScalableVector		GetCameraPosition();
	virtual CScalableVector		GetCameraTarget();
	virtual TVector				GetCameraUp();
	virtual float				GetCameraFOV();

	virtual float				GetCameraNear();
	virtual float				GetCameraFar();

	virtual CScalableVector		GetThirdPersonCameraPosition();
	virtual CScalableVector		GetThirdPersonCameraTarget();

	virtual void				KeyDown(int c);

	void						ToggleThirdPerson();
	void						SetThirdPerson(bool bOn) { m_bThirdPerson = bOn; };
	bool						GetThirdPerson() { return m_bThirdPerson; };

protected:
	bool						m_bThirdPerson;
};

#endif
