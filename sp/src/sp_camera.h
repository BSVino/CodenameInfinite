#ifndef SP_CAMERA_H
#define SP_CAMERA_H

#include <game/entities/camera.h>
#include <common.h>

#include "sp_common.h"

class CSPCamera : public CCamera
{
	REGISTER_ENTITY_CLASS(CSPCamera, CCamera);

public:
								CSPCamera();

public:
	virtual void                CameraThink();

	virtual const TVector       GetUpVector() const;
	virtual float				GetCameraFOV();

	virtual float				GetCameraNear();
	virtual float				GetCameraFar();

	virtual CScalableVector		GetThirdPersonCameraPosition();
	virtual Vector              GetThirdPersonCameraDirection();

	virtual void				KeyDown(int c);

	void						ToggleThirdPerson();
	void						SetThirdPerson(bool bOn) { m_bThirdPerson = bOn; };
	bool						GetThirdPerson() { return m_bThirdPerson; };

	void						SetCharacter(class CSPCharacter* pCharacter);

protected:
	bool						m_bThirdPerson;

	CEntityHandle<CSPCharacter> m_hCharacter;
};

#endif
