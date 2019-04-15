#pragma once
#include <Windows.h>

void* FindPattern(char* pBaseAddress, const char* pbMask, const char* pszMask, size_t nLength);
HMODULE WINAPI GetRemoteModuleHandle(HANDLE hProcess, LPCSTR lpModuleName);
void GetModuleLen(HANDLE hProcess, HMODULE hModule, size_t* addr, size_t* len);
FARPROC WINAPI GetRemoteProcAddress(HANDLE hProcess, HMODULE hModule, LPCSTR lpProcName, UINT Ordinal, BOOL UseOrdinal);
