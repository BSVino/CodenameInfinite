#include "items.h"

static const char* g_szItemNames[] = {
	"None",
	"Dirt",
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
