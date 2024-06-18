#include <unistd.h>
#include <cstring>
#include <sys/socket.h>

#include "common.hpp"

using namespace std;



Exception::Exception(const string& msg_) : msg(msg_) { }
const char* Exception::what() const throw () { return msg.data(); }

bool check_num(string str) {
    for (char c : str) {
        if (!isdigit(c)) 
            return false;
    }
    return true;
}

// We designed the communication protocol in such a way that when recv/send 
// return 0 the socket has unexpectedly closed (by CTRL + C or SIGKILL etc.) 
// Therefore, throw an execption which is caught by the caller and handled appropriatelly
void socket_read(int sock, void* address_, int size) {
    int ret;
    char* address = (char*)address_;

    while (size > 0) {
        if ((ret = recv(sock, address, size, 0)) <= 0 && errno != EINTR)
            throw Exception("socket_read()");

        size -= ret;
        address += ret;
    }
}

void socket_write(int sock, void* address_, int size) {
    int ret;
    char* address = (char*)address_;

    while (size > 0) {
        if ((ret = send(sock, address, size, MSG_NOSIGNAL)) <= 0 && errno != EINTR)
            throw Exception("socket_write()");

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
    TRY_CATCH(
        socket_read(sock, str, len);
        string msg(str);
        delete [] str;
        return msg;,
        delete [] str;
        throw e;
    )
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

    size_t i = 0;

    TRY_CATCH(
        for (; i < *argc; i++) {
            argv[i] = NULL;

            size_t len;
            socket_read(sock, &len, sizeof(len)); // Read bytes of each arg, contains '\0'

            argv[i] = new char[len];
            socket_read(sock, argv[i], len);
        },
        
        for (size_t j = 0; j <= i; j++)
            delete [] argv[j];

        delete [] argv;
        throw e;
    )

    return argv;
}
