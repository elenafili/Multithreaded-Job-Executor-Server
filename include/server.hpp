#pragma once
#include <iostream>
#include <pthread.h>

class Server {
    private:
        int sock;
        uint16_t port;
        size_t buffer_sz;
        size_t thread_pool_sz;
        size_t concurrency;
        pthread_mutex_t lock;

        void parse_argv(char** argv);
        static void* handle_command(void* args); 

        void issue_job(int sock);
        void set_concurrency(int sock);
        void stop_job(int sock);
        void poll(int sock);
        void exit(int sock);
    public:
        Server(size_t argc_, char** argv);
        ~Server();
        void run();
};