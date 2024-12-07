#include <Windows.h>
#include <cstring>
#include <filesystem>
#include <iostream>

typedef bool (*GiveInstruction)(const char*, unsigned short);

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
		GiveInstruction giveInstruction = (GiveInstruction)(GetProcAddress(hInstDLL, "GiveInstruction"));
		bool res = false;
		if (giveInstruction)
		{
			res = giveInstruction("ping", 4);
			if (res)
			{
				if (0 == strcmp(argv[1], "log"))
				{
					HANDLE read = nullptr, write = nullptr;
					CreatePipe(&read, &write, nullptr, 0);

					DWORD pid = GetCurrentProcessId();
					const int size = 4 + sizeof(DWORD) + sizeof(HANDLE);
					char instr[size];

					char* name = instr;
					memcpy(name, "log", 4);

					DWORD* pidPrt = reinterpret_cast<DWORD*>(instr + 4);
					memcpy(pidPrt, &pid, sizeof(DWORD));

					HANDLE* handlePtr = reinterpret_cast<HANDLE*>(instr + 4 + sizeof(DWORD));
					memcpy(handlePtr, &write, sizeof(HANDLE));

					giveInstruction(instr, size);

					while (true)
					{
						char buff[257] = {};
						DWORD bytesRead;
						ReadFile(
							read,
							buff,
							256,
							&bytesRead,
							nullptr
						);

						std::cout << buff << std::endl;
					}
				}
				else {
					giveInstruction(argv[1], strlen(argv[1]));
				}
			}
			else
			{
				if (0 == strcmp(argv[1], "start"))
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
						CREATE_NEW_CONSOLE,
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
		}

		FreeLibrary(hInstDLL);
	}

	return 0;
}
