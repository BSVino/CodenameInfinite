#ifndef SP_CAMERA_H
#define SP_CAMERA_H

#include <game/camera.h>
#include <common.h>

#include "sp_common.h"

class CSPCamera : public CCamera
{
	DECLARE_CLASS(CSPCamera, CCamera);

public:
	virtual CScalableVector		GetCameraScalablePosition();
	virtual CScalableVector		GetCameraScalableTarget();

	virtual Vector				GetCameraPosition();
	virtual Vector				GetCameraTarget();
	virtual Vector				GetCameraUp();
	virtual float				GetCameraFOV();

	virtual float				GetCameraFar() { return 1200.0f; };
};

#endif
