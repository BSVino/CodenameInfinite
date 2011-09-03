#ifndef SP_WINDOW_H
#define SP_WINDOW_H

#include <tinker/gamewindow.h>

class CSPWindow : public CGameWindow
{
	DECLARE_CLASS(CSPWindow, CGameWindow);

public:
								CSPWindow(int argc, char** argv);

public:
	virtual eastl::string		WindowTitle() { return "Codename: Infinite"; }
	virtual tstring				AppDirectory() { return _T("SP"); }

	void						SetScreenshot(size_t iScreenshot) { m_iScreenshot = iScreenshot; };
	void						SetupEngine();
	void						SetupSP();

	virtual void				RenderLoading();

	class CSPRenderer*			GetRenderer();
	class CSPHUD*				GetHUD() { return m_pSPHUD; };

protected:
	size_t						m_iScreenshot;

	class CSPHUD*				m_pSPHUD;
};

inline CSPWindow* SPWindow()
{
	return static_cast<CSPWindow*>(CApplication::Get());
}

#endif
