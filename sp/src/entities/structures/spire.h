#pragma once

#include "structure.h"

class CSpire : public CStructure
{
	REGISTER_ENTITY_CLASS(CSpire, CStructure);

public:
	void        Precache();
	void        Spawn();

	bool        ShouldRender() const;
	void        PostRender() const;

	void        SetBaseName(const tstring& sName) { m_sBaseName = sName; }
	tstring     GetBaseName() { return m_sBaseName; }

private:
	tstring     m_sBaseName;
};
