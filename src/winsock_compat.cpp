#include "winsock_compat.h"
#include <unistd.h>

int closesocket(SOCKET sid) {
    return close(sid);
}

int WSAGetLastError() {
    return errno;
}
