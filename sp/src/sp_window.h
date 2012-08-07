#ifndef SP_WINDOW_H
#define SP_WINDOW_H

#include <ui/gamewindow.h>

class CSPWindow : public CGameWindow
{
	DECLARE_CLASS(CSPWindow, CGameWindow);

public:
								CSPWindow(int argc, char** argv);

public:
	virtual tstring             WindowTitle() { return "Codename: Infinite"; }
	virtual tstring				AppDirectory() { return "SP"; }

	class CRenderer*            CreateRenderer();

	void						SetScreenshot(size_t iScreenshot) { m_iScreenshot = iScreenshot; };
	void						SetupEngine();

	virtual void				RenderLoading();

	class CSPRenderer*			GetRenderer();
	class CSPHUD*				GetHUD();

protected:
	size_t						m_iScreenshot;
};

inline CSPWindow* SPWindow()
{
	return static_cast<CSPWindow*>(CApplication::Get());
}

#endif
