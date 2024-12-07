#include <Windows.h>

enum WinOp
{
	None,
	Created,
	Destroyed,
	Minimized,
	Restored,
};

struct WindowInfo
{
	WinOp m_op = None;
	HWND m_handle = nullptr;
	DWORD m_parentProc = -1;
};