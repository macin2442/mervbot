#ifndef WINDOWS_COMPAT_H
#define WINDOWS_COMPAT_H

#include <cstdint>
#include <dlfcn.h>

#define __stdcall


typedef uint16_t WORD;
typedef uint32_t DWORD;

// TODO remove HKEY, HMODULE
typedef uint32_t HKEY;
typedef void* HMODULE;

uint32_t GetCurrentDirectory(uint32_t nBufferLength, char *lpBuffer);
uint32_t GetPrivateProfileString(const char *lpAppName, const char *lpKeyName, const char *lpDefault, char *lpReturnedString, uint32_t nSize, const char *lpFileName);

void *GetProcAddress(HMODULE module, const char *symbol);
bool FreeLibrary(HMODULE module);

#endif	// WINDOWS_COMPAT_H
