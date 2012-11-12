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

	virtual size_t  MaxDepth() const { return 40; }

	structure_type    StructureType() const { return STRUCTURE_MINE; }

private:
	double          m_flDiggingStarted;

	tvector<item_t> m_aeItemsAtDepth;
	size_t          m_iCurrentDepth;
};
