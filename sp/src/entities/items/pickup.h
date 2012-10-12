#pragma once

#include <game/entities/baseentity.h>

#include "items.h"

class CPickup : public CBaseEntity
{
	REGISTER_ENTITY_CLASS(CPickup, CBaseEntity);

public:
	void        Precache();

	void        Spawn();
	void        Think();

	bool        ShouldRender() const;
	void        PostRender() const;

	void        SetItem(item_t eItem);
	item_t      GetItem() const { return m_eItem; }

private:
	item_t      m_eItem;

	double      m_flNextPlayerCheck;
};

