#ifndef WINDOWS_COMPAT_H
#define WINDOWS_COMPAT_H

#include <cstdint>
#include <cstring>
#include <dlfcn.h>

#define ZeroMemory(p, n) std::memset(p, 0, n)

typedef int BOOL;
typedef uint8_t BYTE;
typedef int16_t SHORT;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef uint32_t HKEY;
typedef void* HMODULE;

enum VersionPlatformId
{
    VER_PLATFORM_WIN32_NT = 2
};

enum RegistryPredefinedKeys
{
    HKEY_CLASSES_ROOT = 0,
    HKEY_CURRENT_CONFIG = 1,
    HKEY_CURRENT_USER = 2,
    HKEY_LOCAL_MACHINE = 3,
    HKEY_USERS = 4
};

enum RegistryValueTypes
{
    REG_BINARY,
    REG_DWORD,
    REG_DWORD_LITTLE_ENDIAN,
    REG_DWORD_BIG_ENDIAN,
    REG_EXPAND_SZ,
    REG_LINK,
    REG_MULTI_SZ,
    REG_NONE,
    REG_QWORD,
    REG_QWORD_LITTLE_ENDIAN,
    REG_SZ
};

enum ErrorCodes {
    ERROR_SUCCESS = 0,
    ERROR_INVALID_FUNCTION = 1,
    ERROR_FILE_NOT_FOUND = 2, 
    ERROR_PATH_NOT_FOUND = 3, 
    ERROR_TOO_MANY_OPEN_FILES = 4,
    ERROR_ACCESS_DENIED = 5,
    ERROR_INVALID_HANDLE = 6,
    ERROR_BAD_ARGUMENTS = 160,
    ERROR_MORE_DATA = 234
};

int RegOpenKey(HKEY hKey, const char *lpSubKey, HKEY *phkResult);
int RegCloseKey(HKEY hKey);
int RegQueryValueEx(HKEY hKey, const char *lpValueName, DWORD *lpReserved, DWORD *lpType, BYTE *lpData, DWORD *lpcbData);
int RegCreateKey(HKEY hKey, const char *lpSubKey, HKEY *phkResult);
int RegSetValueEx(HKEY hKey, const char *lpValueName, DWORD Reserved, DWORD dwType, const BYTE *lpData, DWORD cbData);

int kbhit();

void Sleep(DWORD dwMilliseconds);

uint32_t GetTickCount();

uint32_t GetCurrentDirectory(uint32_t nBufferLength, char *lpBuffer);
DWORD GetPrivateProfileString(const char *lpAppName, const char *lpKeyName, const char *lpDefault, char *lpReturnedString, uint32_t nSize, const char *lpFileName);
BOOL WritePrivateProfileString(const char *lpAppName, const char *lpKeyName, const char *lpString, const char *lpFileName);

void *GetProcAddress(HMODULE module, const char *symbol);
bool FreeLibrary(HMODULE module);

#endif	// WINDOWS_COMPAT_H
