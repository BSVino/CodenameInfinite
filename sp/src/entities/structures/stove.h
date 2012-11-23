#pragma once

#include "structure.h"

class CStove : public CStructure
{
	REGISTER_ENTITY_CLASS(CStove, CStructure);

public:
	void        Precache();
	void        Spawn();

	void        Think();

	bool        ShouldRender() const;
	void        PostRender() const;

	void        SetupMenuButtons();
	void        PerformStructureTask(CSPCharacter* pCharacter);
	bool        IsOccupied() const;

	bool        IsBurning() const { return m_flBurnWoodStart > 0; }

	bool        TakesBlocks() const { return true; }
	size_t      TakeBlocks(item_t eBlock, size_t iNumber);

	structure_type    StructureType() const { return STRUCTURE_STOVE; }

private:
	size_t      m_iBatteryLevel;
	size_t      m_iMaxBatteryLevel;

	double      m_flBurnWoodStart;
};
