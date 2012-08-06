#ifndef SP_HUD_H
#define SP_HUD_H

#include <tengine/ui/hudviewport.h>

class CSPHUD : public CHUDViewport
{
public:
					CSPHUD();

public:
	virtual void	Paint(int x, int y, int w, int h);
};

#endif
