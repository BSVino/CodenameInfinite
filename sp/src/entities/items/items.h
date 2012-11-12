#pragma once

typedef enum
{
	ITEM_NONE = 0,

	// Block types
	ITEM_DIRT,
	ITEM_STONE,

	ITEM_BLOCKS_TOTAL,

	ITEM_TOTAL,
} item_t;

const char* GetItemName(item_t eItem);

const char* GetItemMaterial(item_t eItem);
