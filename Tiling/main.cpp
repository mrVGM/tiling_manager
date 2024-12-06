#include <Windows.h>
#include <cstring>
#include <filesystem>
#include <iostream>

typedef bool (*GiveInstruction)(const char*, int);

int main(int args, const char** argv)
{
	const char* bridgeDLL = "Bridge.dll";
	const char* managerEXE = "Manager.exe";

	if (args < 2)
	{
		return 1;
	}

	HINSTANCE hInstDLL = LoadLibrary(bridgeDLL);
	if (hInstDLL)
	{
		DWORD pid = GetCurrentProcessId();
		GiveInstruction checkMem = (GiveInstruction)(GetProcAddress(hInstDLL, "GiveInstruction"));
		bool res = false;
		if (checkMem)
		{
			int size = strlen(argv[1]) + 1;
			res = checkMem(argv[1], size);

			if (!res && 0 == strcmp(argv[1], "start"))
			{
				STARTUPINFO si;
				PROCESS_INFORMATION pi;

				ZeroMemory(&si, sizeof(si));
				si.cb = sizeof(si);
				si.dwFlags = STARTF_USESHOWWINDOW;
				si.wShowWindow = SW_HIDE;


				std::filesystem::path curPath = std::filesystem::current_path();
				curPath = curPath.append(managerEXE);
				std::string cmd = "cmd /c " + curPath.generic_string();

				if (CreateProcess(
					NULL,
					const_cast<char*>(cmd.c_str()),
					NULL,
					NULL,
					FALSE,
					0,
					NULL,
					NULL,
					&si,
					&pi))
				{
					CloseHandle(pi.hProcess);
					CloseHandle(pi.hThread);
					std::cout << "Tiling Manager running!" << std::endl;
				}
			}
		}

		FreeLibrary(hInstDLL);
	}

	return 0;
}
