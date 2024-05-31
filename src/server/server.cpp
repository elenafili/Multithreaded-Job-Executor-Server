#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <ctype.h>
#include "common.hpp"
#include "server.hpp"

#define PORT 1234

Server::Server() {

    ASSERT_NEQ(sock = socket(AF_INET, SOCK_STREAM, 0), -1);

    int temp = 1;
    ASSERT_NEQ(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &temp, sizeof(int)), -1);

    struct sockaddr_in server;

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(PORT);

    ASSERT_NEQ(bind(sock, (struct sockaddr*) &server, sizeof(server)), -1)
    ASSERT_NEQ(listen(sock, 1 << 8) , -1);
}

Server::~Server() {
    ASSERT_NEQ(close(sock), -1);
}

void Server::run() {
    int newsock;

    while(1) {
        ASSERT_NEQ(newsock = accept(sock, NULL, NULL), -1)
        char buf[1024] = { 0 };

        if (read(newsock, buf, 1024 - 1)) 
            printf("%s\n", buf);
        
        ASSERT_NEQ(close(newsock), -1);
    }
}