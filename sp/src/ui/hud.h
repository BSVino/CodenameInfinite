#ifndef SP_HUD_H
#define SP_HUD_H

#include <tengine/ui/hudviewport.h>

class CSPHUD : public CHUDViewport
{
public:
					CSPHUD();

public:
	virtual void    Paint(float x, float y, float w, float h);

	virtual void    Debug_Paint();
};

#endif
