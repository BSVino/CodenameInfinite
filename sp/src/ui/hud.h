#ifndef SP_HUD_H
#define SP_HUD_H

#include <tengine/ui/hudviewport.h>

class CHUDMenu;

class CSPHUD : public CHUDViewport, public glgui::IEventListener
{
	DECLARE_CLASS(CSPHUD, CHUDViewport);

public:
					CSPHUD();

public:
	virtual void    BuildMenus();

	virtual void    Paint(float x, float y, float w, float h);

	EVENT_CALLBACK(CSPHUD, ConstructSpire);
	EVENT_CALLBACK(CSPHUD, ConstructMine);

	virtual void    Debug_Paint();

private:
	glgui::CControl<CHUDMenu>    m_hConstructionMenu;
};

#endif
