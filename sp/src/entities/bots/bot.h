#pragma once

#include "entities/sp_character.h"

class CBot : public CSPCharacter
{
	REGISTER_ENTITY_CLASS(CBot, CSPCharacter);

public:
	virtual bool                ShouldRender() const { return true; }

	virtual bool                ApplyGravity() const { return false; }
};
