#include "winsock_compat.h"

int closesocket(SOCKET sid) {
    return close(sid);
}

int WSAGetLastError() {
    return errno;
}