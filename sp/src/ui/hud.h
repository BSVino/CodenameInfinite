#ifndef SP_HUD_H
#define SP_HUD_H

#include <tengine/ui/hudviewport.h>

class CHUDMenu;

typedef enum
{
	MENU_EQUIP = 0,
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
	EVENT_CALLBACK(CSPHUD, DesignateBlock);
	EVENT_CALLBACK(CSPHUD, ConstructSpire);
	EVENT_CALLBACK(CSPHUD, ConstructMine);
	EVENT_CALLBACK(CSPHUD, ConstructPallet);

	virtual void    Debug_Paint(float w, float h);

private:
	glgui::CControl<CHUDMenu>    m_ahMenus[MENU_TOTAL];
};

#endif
