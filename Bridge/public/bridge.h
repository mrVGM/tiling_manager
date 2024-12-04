#include <Windows.h>

struct WindowInfo
{
	char m_op = 0;
	HWND m_handle = nullptr;
	bool m_fullscreen = false;
};