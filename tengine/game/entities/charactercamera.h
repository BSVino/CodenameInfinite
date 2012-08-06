#pragma once

#include <common.h>

#include "camera.h"

class CCharacter;

class CCharacterCamera : public CCamera
{
	REGISTER_ENTITY_CLASS(CCharacterCamera, CCamera);

public:
				CCharacterCamera();

public:
	virtual void			CameraThink();

	virtual TVector			GetThirdPersonCameraPosition();
	virtual TVector			GetThirdPersonCameraDirection();

	virtual bool			KeyDown(int c);

	void					SetThirdPerson(bool bOn) { m_bThirdPerson = bOn; };
	bool					GetThirdPerson() { return m_bThirdPerson; };

public:
	bool						m_bThirdPerson;
	CEntityHandle<CCharacter>	m_hCharacter;

	float					m_flBack;
	float					m_flUp;
	float					m_flSide;
};
