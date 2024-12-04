#include "tasks.h"
#include "bridge.h"

#include <iostream>

#include <threads.h>
#include <queue>
#include <functional>
#include <Windows.h>
#include <AclAPI.h>
#include <set>

typedef void (*SetHookFunc)(HINSTANCE, DWORD pid, HANDLE writeHandle, HANDLE instructionHandle);
typedef void (*UnhookFunc)();

HINSTANCE hInstDLL;
SetHookFunc SetHook;
UnhookFunc Unhook;

void InstallHook(HANDLE writePipe, HANDLE instructionPipe)
{
#if DEBUG
	const char* bridgeDLL = "..\\Bridge\\Bridge.dll";
#else
	const char* bridgeDLL = "Bridge.dll";
#endif

	hInstDLL = LoadLibrary(bridgeDLL);
	if (hInstDLL)
	{
		DWORD pid = GetCurrentProcessId();
		SetHook = (SetHookFunc)GetProcAddress(hInstDLL, "SetHook");
		Unhook = (UnhookFunc)GetProcAddress(hInstDLL, "Unhook");
		if (SetHook && Unhook)
		{
			SetHook(hInstDLL, GetCurrentProcessId(), writePipe, instructionPipe);
		}
	}
}

void UninstallHook()
{
	if (Unhook)
	{
		Unhook();
	}
	if (hInstDLL)
	{
		FreeLibrary(hInstDLL);
	}
}

bool ShouldProcessWindow(DWORD style)
{
	if (style & WS_MAXIMIZE)
	{
		return true;
	}

	if (style & WS_SIZEBOX)
	{
		return true;
	}

	return false;
}

int main(int args, const char** argv)
{
	ThreadPool threadPool(3);

	MPSCChannel<bool> channel;

	HANDLE readPipe = nullptr, writePipe = nullptr;
	CreatePipe(&readPipe, &writePipe, nullptr, 0);
	
	HANDLE readInstructionPipe = nullptr, writeInstructionPipe = nullptr;
	CreatePipe(&readInstructionPipe, &writeInstructionPipe, nullptr, 0);

	std::cout << GetLastErrorAsString() << std::endl;
	threadPool.RunTask([&]() {
		char buff[256];

		DWORD read;
		while (true)
		{
			ReadFile(
				readInstructionPipe,
				buff,
				256,
				&read,
				nullptr
			);

			std::string instruction = buff;

			if (instruction == "stop")
			{
				channel.Push(true);
			}
		}
	});

	InstallHook(writePipe, writeInstructionPipe);

	MPSCChannel<WindowInfo> windowsChannel;
	
	threadPool.RunTask([&]() {
		while(true)
		{
			DWORD read;
			WindowInfo winfo;
			ReadFile(
				readPipe,
				&winfo,
				sizeof(WindowInfo),
				&read,
				nullptr
			);

			bool shouldProcess = false;
			DWORD style = GetWindowLong(winfo.m_handle, GWL_STYLE);
			if (ShouldProcessWindow(style))
			{
				windowsChannel.Push(winfo);
			}
		}
	});

	threadPool.RunTask([&]() {
		std::set<HWND> activeWindows;
		while (true)
		{
			WindowInfo winfo = windowsChannel.Pop();

			if (winfo.m_op == 'c')
			{
				activeWindows.insert(winfo.m_handle);
			}
			else if (winfo.m_op == 'd')
			{
				activeWindows.erase(winfo.m_handle);
			}
			if (activeWindows.size() == 0)
			{
				continue;
			}

			int width = 1920 / activeWindows.size();
			int cur = 0;

			for (auto it = activeWindows.begin(); it != activeWindows.end(); ++it)
			{
				DWORD style = GetWindowLong(*it, GWL_STYLE);
				style = style & ~WS_MAXIMIZE;
				SetWindowLong(*it, GWL_STYLE, style | WS_OVERLAPPED);

				WINDOWPLACEMENT wp;
				wp.length = sizeof(wp);

				GetWindowPlacement(*it, &wp);
				wp.rcNormalPosition = { cur, 0, cur + width, 1000};
				wp.ptMinPosition = { cur, 0 };
				wp.ptMaxPosition = { cur + width, 1000 };
				SetWindowPlacement(
					*it,
					&wp
				);
				
				cur += width;
			}
		}
	});
	

	channel.Pop();

	UninstallHook();
 
	//CloseHandle(readPipe);
	//CloseHandle(writePipe);

	return 0;
}
