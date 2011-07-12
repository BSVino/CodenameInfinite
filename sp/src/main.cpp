#include "sp_window.h"

void CreateApplication(int argc, char** argv)
{
	CSPWindow oWindow(argc, argv);

	oWindow.OpenWindow();
	oWindow.SetupEngine();
	oWindow.Run();
}

int main(int argc, char** argv)
{
	CreateApplicationWithErrorHandling(CreateApplication, argc, argv);
}
