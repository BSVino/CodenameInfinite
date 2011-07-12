#ifndef SP_CAMERA_H
#define SP_CAMERA_H

#include <game/camera.h>
#include <common.h>

class CSPCamera : public CCamera
{
	DECLARE_CLASS(CSPCamera, CCamera);

public:
	virtual Vector	GetCameraPosition();
	virtual Vector	GetCameraTarget();
	virtual Vector	GetCameraUp();
	virtual float	GetCameraFOV();
};

#endif
