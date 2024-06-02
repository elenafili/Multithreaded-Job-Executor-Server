#include <unistd.h>
#include <cstring>
#include "common.hpp"

using namespace std;

bool check_num(string str) {
    for (char c : str) {
        if (!isdigit(c)) 
            return false;
    }
    return true;
}

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

void send_msg(int sock, string msg) {
    size_t len = msg.length() + 1;
    socket_write(sock, &len, sizeof(len));
    socket_write(sock, msg.data(), len);
}

string receive_msg(int sock) {
    size_t len;
    socket_read(sock, &len, sizeof(len));

    char* str = new char[len];
    socket_read(sock, str, len);

    string msg(str);
    delete [] str;

    return msg;
}

void send_args(int sock, size_t argc, char** argv) {
    socket_write(sock, &argc, sizeof(argc)); // write number of args

    for (size_t i = 0; i < argc; i++) {
        size_t len = strlen(argv[i]) + 1;

        socket_write(sock, &len, sizeof(len)); // send bytes of each arg
        socket_write(sock, argv[i], len); // send arg
    }
}

char** receive_args(int sock, size_t* argc) {
    socket_read(sock, argc, sizeof(*argc)); // Read number of arguments

    char** argv = new char*[*argc + 1];
    argv[*argc] = NULL;

    for (size_t i = 0; i < *argc; i++) {
        size_t len;
        socket_read(sock, &len, sizeof(len)); // Read bytes of each arg, contains '\0'

        argv[i] = new char[len];
        socket_read(sock, argv[i], len);
    }

    return argv;
}
