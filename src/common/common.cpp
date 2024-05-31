#include <unistd.h>
#include <cstring>
#include "common.hpp"

void socket_read(int sock, void* address_, int size) {
    int ret;
    char* address = (char*)address_;

    while (size > 0) {
        ASSERT_NEQ(ret = read(sock, address, size), -1);
        size -= ret;
        address += ret;
    }
}

void socket_write(int sock, void* address_, int size) {
    int ret;
    char* address = (char*)address_;

    while (size > 0) {
        ASSERT_NEQ(ret = write(sock, address, size), -1);
        size -= ret;
        address += ret;
    }
}

void send_args(int sock, size_t argc, char** argv) {
    socket_write(sock, &argc, sizeof(argc)); // write number of args

    for (size_t i = 0; i < argc; i++) {
        size_t len = strlen(argv[i]) + 1;

        socket_write(sock, &len, sizeof(len)); // send bytes of each arg
        socket_write(sock, argv[i], len); // send arg
    }
}
