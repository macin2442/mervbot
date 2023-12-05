#ifndef WINSOCK_COMPAT_H
#define WINSOCK_COMPAT_H

#include <cstdint>
#include <errno.h>
#include <cstring>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>


typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef int SOCKET;
const int SOCKET_ERROR = -1;

int closesocket(SOCKET sid);

int WSAGetLastError();

#endif	// WINSOCK_COMPAT_H
