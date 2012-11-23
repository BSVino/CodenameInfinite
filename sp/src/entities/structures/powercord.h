#pragma once

class CStructure;

#include <tengine/game/entities/baseentity.h>

class CPowerCord : public CBaseEntity
{
	REGISTER_ENTITY_CLASS(CPowerCord, CBaseEntity);

public:
	void        Precache();
	void        Spawn();

	bool        ShouldRender() const;
	void        PostRender() const;

	void        SetSource(CStructure* pSource);
	void        SetDestination(CStructure* pDestination);

	CStructure* GetSource() const;

private:
	CEntityHandle<CStructure>  m_hSource;
	CEntityHandle<CStructure>  m_hDestination;
};
