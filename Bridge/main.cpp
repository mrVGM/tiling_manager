// HookProcDLL.cpp 
#include "tasks.h"
#include "bridge.h"

#include <windows.h>
#include <fstream>
#include <sstream>
#include <iostream>

#define SHMEMSIZE 128 

static LPVOID lpvMem = NULL;      // pointer to shared memory
static HANDLE hMapObject = NULL;  // handle to file mapping

typedef LRESULT (*HookProc)(int nCode, WPARAM wParam, LPARAM lParam);

HHOOK hHook = nullptr;
HANDLE writeHandle = nullptr;

struct IPCInfo
{
	DWORD PID = -1;
	HANDLE hWrite = nullptr;
	HANDLE hInstruction = nullptr;
};

// DLL entry point 
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{ 
	BOOL fInit, fIgnore;

	switch (ul_reason_for_call) { 
	case DLL_PROCESS_ATTACH: 
		// Create a named file mapping object
		hMapObject = CreateFileMapping(
			INVALID_HANDLE_VALUE,   // use paging file
			NULL,                   // default security attributes
			PAGE_READWRITE,         // read/write access
			0,                      // size: high 32-bits
			SHMEMSIZE,              // size: low 32-bits
			TEXT("dllmemfilemap")); // name of map object
		if (hMapObject == NULL)
			return FALSE;

		fInit = (GetLastError() != ERROR_ALREADY_EXISTS);

		// Get a pointer to the file-mapped shared memory

		lpvMem = MapViewOfFile(
			hMapObject,     // object to map view of
			FILE_MAP_WRITE, // read/write access
			0,              // high offset:  map from
			0,              // low offset:   beginning
			0);             // default: map entire file
		if (lpvMem == NULL)
			return FALSE;

		// Initialize memory if this is the first process

		if (fInit)
			memset(lpvMem, '\0', SHMEMSIZE);

		break;

	case DLL_THREAD_ATTACH: break;
	case DLL_THREAD_DETACH: break;
	case DLL_PROCESS_DETACH:
		// Unmap shared memory from the process's address space

		fIgnore = UnmapViewOfFile(lpvMem);

		// Close the process's handle to the file-mapping object

		fIgnore = CloseHandle(hMapObject);

		break;
	}
	return TRUE;
}

LRESULT CALLBACK MyHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (!writeHandle)
	{
		IPCInfo* info = static_cast<IPCInfo*>(lpvMem);
		HANDLE procHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, info->PID);
		DuplicateHandle(procHandle, info->hWrite, GetCurrentProcess(), &writeHandle, DUPLICATE_SAME_ACCESS, 0, 0);
		CloseHandle(procHandle);
	}
	LRESULT res = CallNextHookEx(hHook, nCode, wParam, lParam);

	if (nCode == HSHELL_WINDOWCREATED)
	{
		WindowInfo winfo;
		winfo.m_handle = reinterpret_cast<HWND>(wParam);
		winfo.m_fullscreen = static_cast<bool>(lParam);
		winfo.m_op = 'c';

		DWORD written;
		WriteFile(
			writeHandle,
			&winfo,
			sizeof(WindowInfo),
			&written,
			nullptr
		);
	}
	else if (nCode == HSHELL_WINDOWDESTROYED)
	{
		WindowInfo winfo;
		winfo.m_handle = reinterpret_cast<HWND>(wParam);
		winfo.m_fullscreen = static_cast<bool>(lParam);
		winfo.m_op = 'd';

		DWORD written;
		WriteFile(
			writeHandle,
			&winfo,
			sizeof(WindowInfo),
			&written,
			nullptr
		);
	}

	return res;
}

extern "C" __declspec(dllexport) void SetHook(HINSTANCE hInstance, DWORD pid, HANDLE hWrite, HANDLE instructionWrite)
{
	IPCInfo* mem = static_cast<IPCInfo*>(lpvMem);
	mem->PID = pid;
	mem->hWrite = hWrite;
	mem->hInstruction = instructionWrite;

	hHook = SetWindowsHookEx(WH_SHELL, MyHookProc, hInstance, 0);
}

extern "C" __declspec(dllexport) void Unhook()
{
	UnhookWindowsHookEx(hHook);
	CloseHandle(writeHandle);
}

extern "C" __declspec(dllexport) bool GiveInstruction(const char* instruction, unsigned short size)
{
	IPCInfo* info = static_cast<IPCInfo*>(lpvMem);
	HANDLE procHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, info->PID);
	HANDLE instructionHandle = nullptr;
	BOOL res = DuplicateHandle(procHandle, info->hInstruction, GetCurrentProcess(), &instructionHandle, DUPLICATE_SAME_ACCESS, 0, 0);
	CloseHandle(procHandle);

	if (res)
	{
		DWORD written;
		WriteFile(
			instructionHandle,
			&size,
			sizeof(size),
			&written,
			nullptr
		);

		WriteFile(
			instructionHandle,
			instruction,
			size,
			&written,
			nullptr
		);
		CloseHandle(instructionHandle);
	}

	return res;
}

