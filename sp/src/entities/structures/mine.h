#pragma once

#include "structure.h"

class CMine : public CStructure
{
	REGISTER_ENTITY_CLASS(CMine, CStructure);

public:
	void        Precache();
	void        Spawn();

	void        Think();

	bool        ShouldRender() const;
	void        PostRender() const;

	void        PerformStructureTask(class CSPCharacter* pCharacter);
	bool        IsOccupied() const;

private:
	double      m_flDiggingStarted;
};
