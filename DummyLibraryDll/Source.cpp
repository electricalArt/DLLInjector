#include <windows.h>

BOOL WINAPI DllMain(
	HINSTANCE hinstDLL,
	DWORD fdwReason,
	LPVOID lpvReserved )
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		MessageBox(0, L"The library is successfuly injected", 0, 0);
	}
}
