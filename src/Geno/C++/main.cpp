#include "Debugging/Console.h"
#include "GUI/MainWindow.h"
#include "ThirdParty/GLEW.h"

int main( int /*argc*/, char* /*argv*/[] )
{
	Console console;

	MainWindow::GetInstance().Init();

	while( MainWindow::GetInstance().BeginFrame() )
	{
		MainWindow::GetInstance().EndFrame();
	}

	return 0;
}

#if defined( _WIN32 )

#include <Windows.h>

int WINAPI WinMain( HINSTANCE /*instance*/, HINSTANCE /*prev_instance*/, LPSTR /*cmd_line*/, int /*cmd_show*/ )
{
	return main( __argc, __argv );
}

#endif // _WIN32