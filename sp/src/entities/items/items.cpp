#include "items.h"

static const char* g_szItemNames[] = {
	"None",
	"Dirt",
	"Stone",
	""
};

const char* GetItemName(item_t eItem)
{
	if (eItem < 0)
		return "";

	if (eItem >= ITEM_TOTAL)
		return "";

	return g_szItemNames[eItem];
}


static const char* g_szItemMaterials[] = {
	"",
	"textures/items/dirt.mat",
	"textures/items/stone.mat",
	""
};

const char* GetItemMaterial(item_t eItem)
{
	if (eItem < 0)
		return "";

	if (eItem >= ITEM_TOTAL)
		return "";

	return g_szItemMaterials[eItem];
}
