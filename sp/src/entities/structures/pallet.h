#pragma once

#include "structure.h"

class CPallet : public CStructure
{
	REGISTER_ENTITY_CLASS(CPallet, CStructure);

public:
	void        Precache();
	void        Spawn();

	void        Think();

	bool        ShouldRender() const;
	void        PostRender() const;

	void        PerformStructureTask(CSPCharacter* pCharacter);

	bool        TakesBlocks() const { return true; }
	size_t      GiveBlocks(size_t iNumber, CSPCharacter* pGiveTo);
	size_t      TakeBlocks(item_t eBlock, size_t iNumber);

	item_t      GetBlockType() const { return m_eItem; }
	size_t      GetBlockQuantity() const { return m_iQuantity; }
	int         GetBlockCapacity() const { return PALLET_CAPACITY; }

	structure_type    StructureType() const { return STRUCTURE_PALLET; }

private:
	item_t      m_eItem;
	size_t      m_iQuantity;

	static const int PALLET_CAPACITY;
};
