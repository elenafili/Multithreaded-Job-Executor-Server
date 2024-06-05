#pragma once
#include <iostream>
#include <list>
#include <pthread.h>

typedef struct Job Job;

class Server {
    private:
        int sock;

        uint16_t port;
        size_t buffer_sz;
        size_t thread_pool_sz;

        size_t concurrency;
        size_t running;

        pthread_mutex_t lock;
        pthread_cond_t cond_full;
        pthread_cond_t cond_empty;

        size_t jids;
        std::list<Job> ready_queue;

        void parse_argv(char** argv);
        Job pop();
        void exec_job(Job& job);
        static void* handle_command(void* args);
        static void* worker(void* args);

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