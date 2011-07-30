#ifndef SP_HUD_H
#define SP_HUD_H

#include <glgui/glgui.h>

class CSPHUD : public glgui::CPanel
{
public:
					CSPHUD();

public:
	virtual void	Paint(int x, int y, int w, int h);
};

#endif
