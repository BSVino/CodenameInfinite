#pragma once

#include "entities/sp_character.h"

class CBot : public CSPCharacter
{
	REGISTER_ENTITY_CLASS(CBot, CSPCharacter);

public:
	virtual bool                ApplyGravity() const { return false; }

	void                        SetOwner(CSPPlayer* pOwner) { m_hOwner = pOwner; }
	CSPPlayer*                  GetOwner() const { return m_hOwner; }

private:
	CEntityHandle<CSPPlayer>    m_hOwner;
};
