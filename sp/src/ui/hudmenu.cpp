#include "hudmenu.h"

#include <tinker/keys.h>
#include <glgui/rootpanel.h>

#include "hud.h"

using namespace glgui;

CHUDMenu::CHUDMenu(size_t iIndex, const tstring& sTitle, bool bSubmenu)
	: glgui::CMenu(sTitle, bSubmenu)
{
	m_iIndex = iIndex;
}

void CHUDMenu::Layout()
{
	BaseClass::Layout();

	if (GetParent().Downcast<CRootPanel>())
		SetPos(15, RootPanel()->GetHeight()/2 + m_iIndex * 25);

	EnsureTextFits();

	m_hMenu->SetPos(GetRight() + 15, GetTop());
}

bool CHUDMenu::KeyPressed(int code, bool bCtrlDown)
{
	if (IsVisible())
	{
		if (code == '1' + m_iIndex)
		{
			if (m_pfnClickCallback)
			{
				m_pfnClickCallback(m_pClickListener, sprintf("%d", m_iIndex));
				return true;
			}
			else if (!m_hMenu->IsVisible())
			{
				OpenMenu();
				return true;
			}
		}
		else if (code == TINKER_KEY_ESCAPE)
			CloseMenu();
	}

	return BaseClass::KeyPressed(code, bCtrlDown);
}

void CHUDMenu::Paint(float x, float y, float w, float h)
{
	BaseClass::Paint(x, y, w, h);
}
