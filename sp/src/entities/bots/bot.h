#pragma once

#include "entities/sp_character.h"

class CSpire;

class CBot : public CSPCharacter
{
	REGISTER_ENTITY_CLASS(CBot, CSPCharacter);

public:
	virtual void                OnUse(CBaseEntity* pUser);

	void                        Think();

	void                        PostRender() const;

	virtual void                SetupMenuButtons() {};
	virtual void                MenuCommand(const tstring& sCommand) {};

	virtual bool                IsFlying() const { return true; }

	void                        SetOwner(CSPPlayer* pOwner) { m_hOwner = pOwner; }
	CSPPlayer*                  GetOwner() const { return m_hOwner; }

	void                        SetSpire(CSpire* pOwner);
	CSpire*                     GetSpire() const;

private:
	CEntityHandle<CSPPlayer>    m_hOwner;
	CEntityHandle<CSpire>       m_hSpire;
};
