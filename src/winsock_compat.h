#ifndef WINSOCK_COMPAT_H
#define WINSOCK_COMPAT_H

#include <cstdint>
#include <errno.h>
#include <cstring>
#include <sys/select.h>

typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef int SOCKET;

int closesocket(SOCKET sid);

int WSAGetLastError();

#endif	// WINSOCK_COMPAT_H
