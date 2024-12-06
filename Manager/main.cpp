#include "tasks.h"
#include "bridge.h"

#include <iostream>

#include <threads.h>
#include <queue>
#include <functional>
#include <Windows.h>
#include <AclAPI.h>
#include <set>
#include <filesystem>
#include <map>

#include "lua.hpp"

typedef void (*SetHookFunc)(HINSTANCE, DWORD pid, HANDLE writeHandle, HANDLE instructionHandle);
typedef void (*UnhookFunc)();

HINSTANCE hInstDLL;
SetHookFunc SetHook;
UnhookFunc Unhook;


void InstallHook(HANDLE writePipe, HANDLE instructionPipe)
{
	const char* bridgeDLL = "Bridge.dll";

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

lua_State* L = nullptr;

int windowId = 0;
std::map<HWND, int> windowToIdMap;
std::map<int, HWND> idToWindowMap;

HANDLE LogWrite = nullptr;

int Log(lua_State* L)
{
	std::string str(lua_tostring(L, -1));
	lua_pop(L, 1);

	if (LogWrite)
	{
		DWORD written;
		WriteFile(
			LogWrite,
			str.c_str(),
			str.size() + 1,
			&written,
			nullptr
		);

		if (written == 0)
		{
			LogWrite = nullptr;
		}
	}

	return 0;
}

int PositionWindow(lua_State* L)
{
	int id = lua_tointeger(L, -5);
	int x = lua_tointeger(L, -4);
	int y = lua_tointeger(L, -3);
	int w = lua_tointeger(L, -2);
	int h = lua_tointeger(L, -1);

	lua_pop(L, 5);

	HWND window = idToWindowMap[id];
	DWORD style = GetWindowLong(window, GWL_STYLE);
	style = style & ~WS_MAXIMIZE;
	SetWindowLong(window, GWL_STYLE, style | WS_OVERLAPPED);

	WINDOWPLACEMENT wp;
	wp.length = sizeof(wp);

	GetWindowPlacement(window, &wp);
	wp.rcNormalPosition = { x, y, x + w, y + h };
	wp.ptMinPosition = { x, y };
	wp.ptMaxPosition = { x + w, y + h };
	SetWindowPlacement(
		window,
		&wp
	);

	return 0;
}

void InitLua()
{
	char moduleFileName[257] = {};
	const char* luaTest = "..\\scripts\\test.lua";
	DWORD size = GetModuleFileName(nullptr, moduleFileName, _countof(moduleFileName) - 1);
	if (size >= _countof(moduleFileName) - 1)
	{
		std::cout << "Path of executable too long!" << std::endl;
		return;
	}

	std::filesystem::path luaTestPath = std::filesystem::path(moduleFileName).parent_path().append(luaTest);
	luaTestPath = std::filesystem::canonical(luaTest);
	std::string luaTestStr = luaTestPath.generic_string();

	L = luaL_newstate();

	lua_pushcfunction(L, PositionWindow);
	lua_setglobal(L, "position_window");
	
	lua_pushcfunction(L, Log);
	lua_setglobal(L, "log");

	luaL_openlibs(L);
	luaL_dofile(L, luaTestStr.c_str());
}

void DeinitLua()
{
	lua_close(L);
}

void LuaWindowCreated(int id)
{
	lua_getglobal(L, "window_created");
	lua_pushinteger(L, id);

	int t = lua_gettop(L);

	lua_pcall(L, 1, 0, 0);
	t = lua_gettop(L);
}

void LuaWindowDestroyed(int id)
{
	lua_getglobal(L, "window_destroyed");
	lua_pushinteger(L, id);

	lua_pcall(L, 1, 0, 0);
}

int main(int args, const char** argv)
{
	InitLua();

	ThreadPool threadPool(3);

	MPSCChannel<bool> channel;

	HANDLE readPipe = nullptr, writePipe = nullptr;
	CreatePipe(&readPipe, &writePipe, nullptr, 0);
	
	HANDLE readInstructionPipe = nullptr, writeInstructionPipe = nullptr;
	CreatePipe(&readInstructionPipe, &writeInstructionPipe, nullptr, 0);

	threadPool.RunTask([&]() {
		char buff[256];

		while (true)
		{
			unsigned short size;

			{
				DWORD read;
				ReadFile(
					readInstructionPipe,
					&size,
					sizeof(size),
					&read,
					nullptr
				);
			}

			char* cur = buff;
			unsigned short left = size;
			while (left > 0)
			{
				DWORD read;
				ReadFile(
					readInstructionPipe,
					cur,
					left,
					&read,
					nullptr
				);
				left -= read;
				cur += read;
			}
			buff[size] = 0;

			std::string instruction = buff;
			if (instruction == "stop")
			{
				channel.Push(true);
			}
			if (instruction == "log")
			{
				DWORD* pid = reinterpret_cast<DWORD*>(buff + 4);
				HANDLE* writeHandle = reinterpret_cast<HANDLE*>(buff + 4 + sizeof(DWORD));

				HANDLE procHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, *pid);
				HANDLE instructionHandle = nullptr;
				BOOL res = DuplicateHandle(procHandle, *writeHandle, GetCurrentProcess(), &LogWrite, DUPLICATE_SAME_ACCESS, 0, 0);
				CloseHandle(procHandle);

				if (!res)
				{
					LogWrite = nullptr;
				}
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
		int windowId = 0;

		while (true)
		{
			WindowInfo winfo = windowsChannel.Pop();

			if (winfo.m_op == 'c')
			{
				windowToIdMap[winfo.m_handle] = windowId;
				idToWindowMap[windowId] = winfo.m_handle;
				LuaWindowCreated(windowId);
				++windowId;
			}
			else if (winfo.m_op == 'd')
			{
				int id = windowToIdMap[winfo.m_handle];
				windowToIdMap.erase(winfo.m_handle);
				idToWindowMap.erase(id);

				LuaWindowDestroyed(id);
			}
			continue;
		}
	});
	

	channel.Pop();

	UninstallHook();
	DeinitLua();
 
	//CloseHandle(readPipe);
	//CloseHandle(writePipe);

	return 0;
}
