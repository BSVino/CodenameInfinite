#ifndef SP_HUD_H
#define SP_HUD_H

#include <tengine/ui/hudviewport.h>

class CHUDMenu;

typedef enum
{
	MENU_WEAPONS = 0,
	MENU_BLOCKS,
	MENU_CONSTRUCTION,
	MENU_HELPER,
	MENU_TOTAL,
} menutype_t;

class CSPHUD : public CHUDViewport, public glgui::IEventListener
{
	DECLARE_CLASS(CSPHUD, CHUDViewport);

public:
					CSPHUD();

public:
	virtual void    BuildMenus();

	virtual void    Paint(float x, float y, float w, float h);

	EVENT_CALLBACK(CSPHUD, PlaceBlock);
	EVENT_CALLBACK(CSPHUD, ConstructSpire);
	EVENT_CALLBACK(CSPHUD, ConstructMine);

	virtual void    Debug_Paint();

private:
	glgui::CControl<CHUDMenu>    m_ahMenus[MENU_TOTAL];
};

#endif
