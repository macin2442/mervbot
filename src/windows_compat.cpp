#include "windows_compat.h"

#include "INIReader.hpp"

uint32_t GetCurrentDirectory(uint32_t nBufferLength, char *lpBuffer) {
	auto cwd = std::filesystem::current_path().string()

	if (lpBuffer == nullptr || nBufferLength == 0) {
		return static_cast<uint32_t>(cwd.size() + 1);
	}

	std::strncpy(lpBuffer, cwd.c_str(), nBufferLength);

	if (nBufferLength < cwd.size() + 1) {
		// buffer too small
		return static_cast<uint32_t>(cwd.size() + 1);
	}
	else {
		// buffer large enough
		return static_cast<uint32_t>(cwd.size());
	}
}

uint32_t GetPrivateProfileString(const char *lpAppName, const char *lpKeyName, const char *lpDefault, char *lpReturnedString, uint32_t nSize, const char *lpFileName) {
	// lpAppName = name of section containing the key name
	// if null -> copy all section names in the file

	// lpKeyName = name of key
	// if null -> copy all key names in the section

	// lpDefault
	// if null -> copy ""

	// use INIreader https://github.com/wme7/INIreader/blob/master/INIreader/main.cpp
	INIReader reader(lpFileName);
	if (lpAppName == nullptr || lpKeyNAME == nullptr)
		throw std::invalid_argument("not implemented");

	const char *defaultValue = (lpDefault == nullptr ? "" : lpDefault);
	std::string value = reader.Get(lpAppName, lpKeyName, lpDefault);
	strncpy(lpReturnedString, nSize, value.c_str());

	if (nSize < value.size() + 1) {
		// buffer too small
		lpReturnedString[nSize - 1] = '\0';
		return nSize - 1;
	}
	else {
		// buffer large enough
		return static_cast<uint32_t>(value.size());
	}
}

void *GetProcAddress(HMODULE module, const char *symbol)
{
	return dlsym(module, symbol);
}

bool FreeLibrary(HMODULE module)
{
	return dlclose(module) != 0;
}
