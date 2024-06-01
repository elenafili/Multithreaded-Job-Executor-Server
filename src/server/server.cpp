#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <ctype.h>
#include <cstring>
#include "common.hpp"
#include "server.hpp"

struct ControllerArgs {
    Server* server;
    int socket;

    ControllerArgs(Server* server_, int socket_) : server(server_), socket(socket_) {}
};

using namespace std;

void Server::parse_argv(char** argv) {
    string str1(argv[1]);
    ASSERT_COND(check_num(str1), "'%s' is not a natural number!\n", argv[1]);
    
    port = atoi(str1.data());

    string str2(argv[2]);
    ASSERT_COND(check_num(str2), "'%s' is not a natural number!\n", argv[2]); 
    ASSERT_GEQ(buffer_sz = atoi(str2.data()), 1);

    string str3(argv[3]);
    ASSERT_COND(check_num(str3), "'%s' is not a natural number!\n", argv[3]); 
    ASSERT_GEQ(thread_pool_sz = atoi(str3.data()), 1);
}

Server::Server(size_t argc_, char** argv) : concurrency(1){
    ASSERT_COND(argc_ == 4, "Usage: %s <portNum> <bufferSize> <threadPoolSize>\n", argv[0]);
    parse_argv(argv);

    ASSERT_NEQ(sock = socket(AF_INET, SOCK_STREAM, 0), -1);

    int temp = 1;
    ASSERT_NEQ(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &temp, sizeof(int)), -1);

    struct sockaddr_in server;

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);

    ASSERT_NEQ(bind(sock, (struct sockaddr*) &server, sizeof(server)), -1)
    ASSERT_NEQ(listen(sock, 1 << 8) , -1);

    ASSERT_EQ(pthread_mutex_init(&lock, NULL), 0);

    // pthread_t worker;

    // for (size_t i = 0; i < thread_pool_sz; i++) {
    //     ASSERT_EQ(pthread_create(&worker, NULL, connection_handler , &conn_desc), 0);
    // }

}

Server::~Server() {
    ASSERT_NEQ(close(sock), -1);
}

void Server::run() {
    int newsock;

    while(1) {

        ASSERT_NEQ(newsock = accept(sock, NULL, NULL), -1)

        pthread_t controller;
        ASSERT_EQ(pthread_create(&controller, NULL, handle_command, new ControllerArgs(this, newsock)), 0);
        ASSERT_EQ(pthread_detach(controller), 0);
    }
}

void* Server::handle_command(void* args) {
    ControllerArgs* cargs = (ControllerArgs*) args;
    CommandType type;
    socket_read(cargs->socket, &type, sizeof(type));

    if (type == ISSUE_JOB)
        cargs->server->issue_job(cargs->socket);
    else if (type == SET_CONCURRENCY)
        cargs->server->set_concurrency(cargs->socket);
    else if (type == STOP_JOB)
        cargs->server->stop_job(cargs->socket);
    else if (type == POLL)
        cargs->server->poll(cargs->socket);
    else 
        cargs->server->exit(cargs->socket);

    ASSERT_NEQ(close(cargs->socket), -1);
    delete cargs;

    return NULL;
}

void Server::issue_job(int sock) {

    // size_t argc;
    // char** argv = receive_args(sock, &argc);

    printf("issue job iou\n");
}

void Server::set_concurrency(int sock) {
    size_t concurrency_;
    socket_read(sock, &concurrency_, sizeof(concurrency_));

    ASSERT_EQ(pthread_mutex_lock(&lock), 0);
    concurrency = concurrency_;
    ASSERT_EQ(pthread_mutex_unlock(&lock), 0);

    string msg = "CONCURRENCY SET AT " + to_string(concurrency_);
    size_t len = msg.length() + 1;

    socket_write(sock, &len, sizeof(len));
    socket_write(sock, msg.data(), len);
}

void Server::stop_job(int sock) {

    // size_t argc;
    // char** argv = receive_args(sock, &argc);

    printf("stop job iou\n");
}

void Server::poll(int sock) {
    printf("poll iou\n");
}

void Server::exit(int sock) {    
    printf("exit iou\n");
}
