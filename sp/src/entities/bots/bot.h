#pragma once

#include "entities/sp_character.h"

class CBot : public CSPCharacter
{
	REGISTER_ENTITY_CLASS(CBot, CSPCharacter);

public:
	virtual void                OnUse(CBaseEntity* pUser);

	void                        Think();

	void                        PostRender() const;

	virtual void                SetupMenuButtons() {};
	virtual void                MenuCommand(const tstring& sCommand) {};

	virtual bool                ApplyGravity() const { return false; }

	void                        SetOwner(CSPPlayer* pOwner) { m_hOwner = pOwner; }
	CSPPlayer*                  GetOwner() const { return m_hOwner; }

private:
	CEntityHandle<CSPPlayer>    m_hOwner;
};
