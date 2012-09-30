#pragma once

#include <glgui/menu.h>

class CHUDMenu : public glgui::CMenu
{
	DECLARE_CLASS(CHUDMenu, glgui::CMenu);

public:
	CHUDMenu(size_t iIndex, const tstring& sTitle, bool bSubmenu = false);

public:
	virtual void    Layout();

	virtual bool    KeyPressed(int code, bool bCtrlDown = false);

	virtual void    Paint(float x, float y, float w, float h);

private:
	size_t          m_iIndex;
};
