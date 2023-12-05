/*
	System interface by cat02e@fsu.edu
*/


#ifndef SYSTEM_H
#define SYSTEM_H

#include "datatypes.h"

#if __linux__
#include "windows_compat.h"
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winreg.h>
#endif

Uint32 getPrivateProfile32(const char *section, const char *key, const char *def, const char *path);

Uint32 getSetting32(HKEY baseKey, const char *path, const char *value, uint32_t default_value);

void setSetting32(HKEY baseKey, const char *path, const char *value, Uint32 buffer);

void getServiceString(HKEY baseKey, const char *path, const char *value, char *buffer);

// Add news checksum to SubSpace news checksum archive in registry
void addNewsChecksum(Uint32 Checksum);

// Works only once! Changes DOS prompt window title
void setWindowTitle(char *title);

// Extract data lines from a mixed-format database file
bool readDataLines(char *file, void (*callback)(char *line));

// Decompress buffer to a file on disk
bool decompress_to_file(const char *name, void *buffer, unsigned long len);

#endif	// SYSTEM_H
