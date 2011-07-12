#ifndef SP_WINDOW_H
#define SP_WINDOW_H

#include <tinker/gamewindow.h>

class CSPWindow : public CGameWindow
{
	DECLARE_CLASS(CSPWindow, CGameWindow);

public:
								CSPWindow(int argc, char** argv);

public:
	virtual eastl::string		WindowTitle() { return "Space Prototype!"; }
	virtual tstring				AppDirectory() { return _T("SP"); }

	void						SetScreenshot(size_t iScreenshot) { m_iScreenshot = iScreenshot; };
	void						SetupEngine();
	void						SetupSP();

	virtual void				RenderLoading();

	virtual void				DoKeyPress(int c);

	class CGeneralWindow*		GetGeneralWindow() { return m_pGeneralWindow; }

	class CSPRenderer*			GetRenderer();

protected:
	size_t						m_iScreenshot;

	class CGeneralWindow*		m_pGeneralWindow;
};

inline CSPWindow* SPWindow()
{
	return static_cast<CSPWindow*>(CApplication::Get());
}

#endif
