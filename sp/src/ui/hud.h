#ifndef SP_HUD_H
#define SP_HUD_H

#include <tengine/ui/hudviewport.h>

class CSPHUD : public CHUDViewport
{
	DECLARE_CLASS(CSPHUD, CHUDViewport);

public:
					CSPHUD();

public:
	virtual void    Paint(float x, float y, float w, float h);

	virtual void    Debug_Paint();
};

#endif
