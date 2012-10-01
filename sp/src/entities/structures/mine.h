#pragma once

#include "structure.h"

class CMine : public CStructure
{
	REGISTER_ENTITY_CLASS(CMine, CStructure);

public:
	void        Precache();
	void        Spawn();

	bool        ShouldRender() const;
	void        PostRender() const;
};
