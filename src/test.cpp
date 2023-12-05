#include "windows_compat.h"
#include <iostream>

int main() {
    std::cout << "Press any key to continue..." << std::endl;
    while(!kbhit());

    /*
    HKEY_CLASSES_ROOT = 0,
    HKEY_CURRENT_CONFIG = 1,
    HKEY_CURRENT_USER = 2,
    HKEY_LOCAL_MACHINE = 3,
    HKEY_USERS = 4
    */


    HKEY hkey;
    int err;

    err = RegOpenKey(HKEY_LOCAL_MACHINE, "foo\\bar", &hkey);
    if (err == ERROR_SUCCESS) {
        std::cerr << "ERROR: Succeeded to open non-existant key HKEY_LOCAL_MACHINE\\foo\\bar." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    
    err = RegCreateKey(HKEY_LOCAL_MACHINE, "bar\\foo", &hkey);
    if (err != ERROR_SUCCESS) {
        std::cerr << "ERROR: Failed to create key HKEY_LOCAL_MACHINE\\bar\\foo." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    DWORD value = 42;
    err = RegSetValueEx(hkey, "baz", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&value), sizeof(value));
    if (err != ERROR_SUCCESS) {
        std::cerr << "ERROR: Failed to set value HKEY_LOCAL_MACHINE\\bar\\foo\\baz." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    value = 0;
    DWORD type;
    DWORD len = 3;
    err = RegQueryValueEx(hkey, "baz", NULL, &type, reinterpret_cast<BYTE*>(&value), &len);
    if (err == ERROR_SUCCESS) {
        std::cerr << "ERROR: Succeeded to query value without too small buffer." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    if (len != sizeof(value)) {
        std::cerr << "ERROR: RegQueryValueEx reported invalid length." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    len = 4;
    err = RegQueryValueEx(hkey, "baz", NULL, &type, reinterpret_cast<BYTE*>(&value), &len);
    if (err != ERROR_SUCCESS) {
        std::cerr << "ERROR: Failed to query existing value." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    if (value != 42 || type != REG_DWORD) {
        std::cerr << "ERROR: Obtained wrong value or type in query." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    const char *text = "poof value with some backslash: \\";
    err = RegSetValueEx(hkey, "poof", 0, REG_SZ, reinterpret_cast<const BYTE*>(text), std::strlen(text) + 1);
    if (err != ERROR_SUCCESS) {
        std::cerr << "ERROR: Failed to set value HKEY_LOCAL_MACHINE\\bar\\foo\\poof." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    char buffer[256];
    len = 0;
    err = RegQueryValueEx(hkey, "poof", NULL, &type, reinterpret_cast<BYTE*>(buffer), &len);
    if (err == ERROR_SUCCESS) {
        std::cerr << "ERROR: Succeeded to query with insufficient buffer." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    if (len != std::strlen(text) + 1) {
        std::cerr << "ERROR: RegQueryValueEx reported incorrect buffer size." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    len = 256;
    err = RegQueryValueEx(hkey, "poof", NULL, &type, reinterpret_cast<BYTE*>(buffer), &len);
    if (type != REG_SZ || len != std::strlen(text) + 1 || strncmp(text, buffer, len) != 0) {
        std::cerr << "ERROR: Obtained wrong value or type in REG_SZ query." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    err = RegCloseKey(42);
    if (err == ERROR_SUCCESS) {
        std::cerr << "ERROR: Succeeded to close invalid key handle." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    err = RegCloseKey(HKEY_LOCAL_MACHINE);
    if (err == ERROR_SUCCESS) {
        std::cerr << "ERROR: Succeeded to close invalid key handle." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    err = RegCloseKey(hkey);
    if (err != ERROR_SUCCESS) {
        std::cerr << "ERROR: Failed to close valid key handle." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    const char *iniFileName = "test.ini";
    const char *iniValue = "value";
    const char *iniDefaultValue = "default";
    BOOL success = WritePrivateProfileString("section", "key", iniValue, iniFileName);
    if (!success) {
        std::cerr << "ERROR: Failed to write key-value pair to ini file." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    len = GetPrivateProfileString("section", "key", iniDefaultValue, buffer, sizeof(buffer), iniFileName);
    if (len != std::strlen(iniValue) || std::strncmp(iniValue, buffer, std::strlen(iniValue)) != 0) {
        std::cerr << "ERROR: Failed to obtain correct key-value pair from ini file." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    len = GetPrivateProfileString("section2", "key", iniDefaultValue, buffer, sizeof(buffer), iniFileName);
    if (len != std::strlen(iniDefaultValue) || std::strncmp(iniDefaultValue, buffer, std::strlen(iniDefaultValue)) != 0) {
        std::cerr << "ERROR: Succeeded to obtain non-existant key-value pair from ini file." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    return 0;
}