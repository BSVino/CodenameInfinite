#pragma once

#include <game/entities/baseentity.h>

class CStatic : public CBaseEntity
{
	REGISTER_ENTITY_CLASS(CStatic, CBaseEntity);

public:
	virtual void		OnSetModel();
};
